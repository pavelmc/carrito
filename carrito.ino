/*
 * Pin mapping
 *
 * *** NRF24L01 Radio ***
 * Radio    Arduino
 * CE    -> 9
 * CSN   -> 10 (Hardware SPI SS)
 * MOSI  -> 11 (Hardware SPI MOSI)
 * MISO  -> 12 (Hardware SPI MISO)
 * SCK   -> 13 (Hardware SPI SCK)
 * IRQ   -> No connection
 * VCC   -> No more than 3.6 volts
 * GND   -> GND
 *
 * *** DIRECTION MOTOR ***
 * pushed via SPI to the 74HC595 on the upper nibble but is uses ...
 * 8 - as SPI CS for the shift register
 *
 * *** POWER MOTOR ***
 * 6 - PWM motor speed
 * 5 - motor direction (low forward, high reverse)
 *
 * *** DIRECTION LIMIT SWITCHES ***
 * A0 - will float in 1/2 V
 * and will fire 0 or 1023 on each switch hit
 *
 * ** CENTER HALL sensor***
 * 2 - Will be low when direction is centered
 *
 * *** ULTRASONIC RADAR ***
 * 5 - trigger
 * 6 - Echo
 *
 */

//~ /***** SONAR *****/
//~ #include <HCSR04.h>
//~ UltraSonicDistanceSensor distanceSensor(5, 6);

/***** NRF24L01 *****/
#include <SPI.h>
#include <NRFLite.h>

// Our radio's id.  The transmitter will send to this id.
const static uint8_t RADIO_ID = 18;
const static uint8_t PIN_RADIO_CE = 9;
const static uint8_t PIN_RADIO_CSN = 10;

/*************************************************************************
 * Radio data structure explained:
 *
 * We will pass just relative values, aka, increments
 *
 * -> Speed: (int16)
 *      motor speed increments, every tap increment it in a fixed val
 *      the same for decrements
 * -> Speed hold (bool)
 *      true if we are holding the actual speed, false if we must set
 *      the motor free (this means that we keep the FWD/REV key pressed)
 * -> Turn: (int16)
 *      Turn direction increments, every tap increment it in a fixed val
 *      the same for decrements
 * -> Turn hold (bool)
 *      True if we are holding any direction key.
 * -> Options; (unsigned long)
 *      really 4 bytes, first one is bit mapped to options as
 *      lights, siren, etc... the others will be worked out
*************************************************************************/
// Any packet up to 32 bytes can be sent.
struct RadioPacket {
    unsigned long serial = 0;
    int speed;
    bool speed_hold;
    int turn;
    bool turn_hold;
    bool stop = 0;
    bool lookAhead = 0;
    unsigned long options;
};

// instantiate
NRFLite _radio;
RadioPacket _radioData;

/***** Stepper motor *****/

#include "stepperunipolar.h"
// instantiate it using
stepperunipolar dirMot(UNIPOLAR_WAVE);


/************************************************************************/

// constants
#define motorSpeedPin     (6)       // pin that will be controlling the
                                    // PWM speed of the motor
#define motorDirectionPin (5)       // set the direction of rotation
#define maxSpeed          (255)     // maximum speed
#define dirLimitPin       (A3)      // Analog pin for limit switches
#define dirHall           (2)       // pin in wich is the front hall
#define dirLatch          (8)       // latch or CS for the shift register

// Variables
volatile bool center = false; // changed by an interrupt, must be reset after read high
unsigned long pktSerial = 0; // serial to sign for duplicated packets
int speed = 0;               // will be mapped from -255 to 0-255
int turn = 0;                // this will be steps in each direction

bool direction = 0;         // 0 is forward, 1 is backwards
float wallDistance = -1.0;  // distance measured from the sensor
                            // to the wall

byte shiftOptions = 0;      // shift register options, you can only set
                            // the lower four, the upper four are the
                            // direction motor control bits

// Serial DEBUG
//~ #define SDEBUG      0

/************************************************************************/

// interrupt rutine
void centerHit() {
    center = true;
}

// set motion speed, just set the PWM value against the limit
void setSpeed(int val) {
    // local var
    byte lval = byte(abs(val));

    // set global var value
    byte lspeed = map(lval, 0, 255, 0, maxSpeed);

    // apply
    analogWrite(motorSpeedPin, lspeed);
}


// set direction of rotatio of the power motor
// 0 is forward, 1 is back
void setDirection(bool dir) {
    // set global var
    direction = dir;
    // apply
    digitalWrite(motorDirectionPin, direction);
}


// stop motion of the motor
void motionStop() {
    // invert the motor direction for a brief time
    // keep the speed
    setDirection(!direction);

    // delay proportional to the speed
    delay(abs(speed));

    // stop the motor, stop speed
    speed = 0;
    // the increment factor depends on inertia and hance on the weight
    // of the car, you must tweak it to suit your needs, mine in
    // bare-bones is about 5, with full chassis must be higher
    setSpeed(abs(speed) * 5);
}

// do turns
void doturn(int t) {
    /** turn only in possible directions **/
    int lPos = checkLimit();

    // hit right and commanded right, not possible
    if (lPos == 1 and t > 0) return;

    // hit left and commanded left, not possible
    if (lPos == -1 and t < 0) return;

    // middle position, do move to either way
    turn = t;
    dirMot.steps(turn);
}


// receive data from the remote control unit
void rxCommand() {
    // data will be set into a buffer and process flag will be rised

    // receive radio data
    while (_radio.hasData()) {
        // Note how '&' must be placed in front of the variable name.
        _radio.readData(&_radioData);

        // reset the serial
        pktSerial = _radioData.serial;

        /**** priority commands first and will exit the loop ****/

        // Stop
        if (_radioData.stop) {
            // stop motor
            motionStop();

            // exit loop
            break;
        }


        /**** normal commands   ****/

        // look ahead
        if (_radioData.lookAhead) {
            // call the procedure tolookahead
            dirTofont();
            // exit loop
            break;
        }

        // set speed to this
        if (_radioData.speed != 0) {
            // calc speed
            speed += _radioData.speed;

            // check limits
            if (speed < -255) speed = -255;
            if (speed > 255) speed = 255;

            // which direction
            if (speed > 0) {
                // forward
                setDirection(0);
            } else {
                // reverse
                setDirection(1);
            }

            // set speed
            setSpeed(speed);
        }

        // set turn direction
        if (_radioData.turn != 0) {
            // doturn
            doturn(_radioData.turn);
        }

        // set options to this
        //~ setOptions(_radioData.options);

        // DEBUG
        #ifdef SDEBUG
        Serial.println(speed);
        Serial.println(direction);
        #endif
    }
}


// check direction limit switches
char checkLimit() {
    // measure value
    int ana = analogRead(dirLimitPin);

    // check limits
    if (ana < 200) return -1;   // left
    if (ana > 823) return  1;   // right

    // if not in range just return zero
    return 0;
}


// Move direction to the front position
void dirTofont() {
    // check if we are already looking forward
    if (digitalRead(dirHall) == 0) return;

    // local var to set the direction
    int steps = 500;
    if (turn > 0) steps = -500;

    // move
    doturn(steps);

    // move to center
    while(!center) {
        // motor move
        dirMot.check();

        // check limit hit
        if (checkLimit() > 0) {
            // right limit hit, move left
            doturn(-500);
        }

        // check limit hit
        if (checkLimit() < 0) {
            // left limit hit, move right
            doturn(500);
        }
    }

    // reset the center flag
    dirMot.stop();
    center = false;

    // once done reset turn to center
    turn = 0;
}


// shift out the motor data for the direction
void dirShiftOut(byte dat) {
    // move the bits to the upper four and mask it with the shiftOptions
    dat = dat << 4;
    shiftOptions = (shiftOptions & B00001111) + dat;

    // shift it out
    shiftOut();
}


// shift the motor and options out
void shiftOut() {
    // pull the CS low
    digitalWrite(dirLatch, LOW);
    // push it
    SPI.transfer(shiftOptions);
    // pull the CS high
    digitalWrite(dirLatch, HIGH);
}

/************************************************************************/

void setup () {
    // serial console
    #ifdef SDEBUG
    Serial.begin(9600);
    #endif

    // init the NRF radio
    /*****
     * By default, 'init' configures the radio to use a 2MBPS bitrate
     * on channel 100 (channels 0-125 are valid).
     *
     * Both the RX and TX radios must have the same bitrate and
     * channel to communicate with each other.
     *
     * You can assign a different bitrate and channel as shown
     * below.
     *
     * _radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN,
     *             NRFLite::BITRATE250KBPS, 0)
     * _radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN,
     *             NRFLite::BITRATE1MBPS, 75)
     * _radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN,
     *             NRFLite::BITRATE2MBPS, 100)
     *******/
    if (!_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE250KBPS, 52)) {
        #ifdef SDEBUG
        Serial.println("Cannot communicate with radio");
        #endif
        while (1); // Wait here forever.
    }

    // Radio initialized ok
    #ifdef SDEBUG
    Serial.println("Radio init done.");
    #endif

    // direction motor
    pinMode(dirLatch, OUTPUT);
    digitalWrite(dirLatch, HIGH);
    // set call back function
    dirMot.addCallBack(dirShiftOut);
    // set speed in steps per one second (1000 Max)
    dirMot.speed(80);
    // set steps per turn
    dirMot.stepsPerTurn = 200;
    // set eco mode, just power during the move, on idle poweroff
    dirMot.eco = true;

    // hal sensor setup (FALLING, it's normally high)
    pinMode(dirHall, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(dirHall), centerHit, FALLING);

    // delay to allow for settling.
    delay(500);

    // face fwd
    dirTofont();

    // set pins related to FWD/REV and speed
    pinMode(motorSpeedPin, OUTPUT);
    pinMode(motorDirectionPin, OUTPUT);

}


void loop () {
    // Every 500 miliseconds, do a measurement using the sensor and print the distance in centimeters.
    //~ #ifdef SDEBUG
    //~ Serial.println(distanceSensor.measureDistanceCm());
    //~ #endif
    //~ delay(500);

    // receive data from remote control
    rxCommand();

    // check async to do the move
    dirMot.check();
}
