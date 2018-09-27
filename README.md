#Carrito (Spanish for Small car)#

![](https://raw.github.com/pavelmc/carrito/master/photos/bare-bones-car.jpg)

This is a homebrewed small car powered by Arduino, there is no particular blueprint for it, the parts comes and go to fit my needs, this evolves continually. Almost all parts were scraped from my junkbox.

You can take a peek at the (always evolving) fritzing file in the "extra" directory (there is also PNG and PDF versions for your convenience)

The main motivations to make this car is the fact that I haven't a cool Arduino project to display... I also confess that I didn't had a remote controlled car as a child so I made this to my children also.

#Dependencies#

If you want to build or test this you will need a few extra things not in this repository:

* The [remote control](https://github.com/pavelmc/carrito-control) device to command this thing.
* A custom lib to command the stepper motor, its called [stepperUnipolar](https://github.com/pavelmc/stepperUnipolar) and has some nice features.
* A library to control the nRF24l01 device, I use the nRFLite lib, you can find it on the Arduino IDE Library manager.

#Actual Hardware#

**Power source:** Laptop batteries; I have become a recycle fan of laptop batteries on any kind for many uses, I have it from 1.2Ah to 2.5Ah, in this case I chose 4 cells is 2P2S mode of 2.2Ah, that gives a total of 6.0 to 8.4 V at up to 4.4 Ah, that's plenty of power for the task.

Every lipo battery need a [BMS](wikipedia:battery_monitor_system) and I borrowed one controller from a Baofeng handheld radio battery pack, it was rated 1.800mAh (1,8Ah) but carefully inspection showed that the MOSFET on the BMS was capable of handling up to 10 amps, so it must be up to the task. (I calculate a peak load of 1.3 Amps top)

_note:_ I charged the battery pack at the design stage and after about a month of bench and field test (about 3 to 4 days per week of tests with ~1 hour per day) it's now at ~7.0 V about 50% of charge; maximum current measured was about 1.8A, nice very nice.

**Motion drive:** Old dotmatrix Epson Printer DC motor and gears commanded by a 6AM14 mosfet Bridge that allows to control the direction (polarity inversion) and speed (via a PWM on a P-channel MOSFET)

**Direction control:** Stepper motor controlling a gear that rotates the front wheels. The motor was borrowed from a old dotmatrix Epson Printer. The motor is unipolar (type 17PM-K303, NEMA17 equivalent from Minebea) controlled by 4 MOSFET borrowed from a PC Motherboard (P75N02, 7.5 milli Ohms and 25 Volt) directly connected to the 74HC595 at 0-3.3V logic.

The motor ratings specs 1.2A on the coil and max of 3.0 Volts, so I fit a positive side current limiter using a PNP darlingtown transistor circuit (2SA1568, again borrowed from the printer, this are the pin controllers on the datMatrix printer)

**Direction artifacts:** The front wheels direction has one problem: it can move and miss steps on bumps and hits, so I need some way to know the position, but all time position are good or only a few ones? powering the stepper is a way to keep the direction in place, but that monster eating 1.2 Amp is not an option.

After analyze the situation I found that I will only need three position reports:

* Full left.
* Mid of front facing.
* Full right.

Left/Right limits was implemented with mouse click switches and also served as hardware limiters, the front facing position as a little tricky (I tried some dancing switches from an old bubblejet printer and other options).

I ended with an Hall effect sensor, it has A42E label but I can't find the datasheet _(if you know what part it's I will appreciate any clue, the initial "A" letter seems like the Allegro one)_, it was borrowed from an old floppy disk drive, per the circuit I can tell that it needs a pull up 10k transistor for 5V and of curse the pinout is evident.

![](https://raw.github.com/pavelmc/carrito/master/photos/limit_switches_hall.jpg)

This hall sensor triggers a pin low once the direction is facing forward, as simple as that. The pull up is by software on the arduino programming, test proved that it works just fine on 3.3V.

**Remote control:** the remote control is designed/made from day one using a pair of nRF24L01 modules at 250 kbps, more than the needed for the task. I tested a lot of libs for this modules and ended using the nrflite one by his final small size firmware and easy programming.

#No Go Hardware & Failed tries#

This is a list of failed tries of hardware and ideas and his explanation (you can find images of the items described here in the folder no-go-and-failures)

##Power inversion for the power motor##

First try was using 12V Relays from a UPS, it consumes only 28mA to drive the coil and can handle up to 10A, driver circuit is just a transistor connected to the Arduino, so it was a nice idea, but not in practice.

![](https://raw.github.com/pavelmc/carrito/master/photos/relay-control.jpg)

Theory some times is ridiculously killed by practice: the 8.4V of power was just enough to activate the 12V relays, but once the voltage of the battery dropped to ~7V it's stops of activate the relays, or activates only one... so no-go on this.

Next obvious solution was a H-Bridge, but P MOSFET transistor are rate here, so I surfed the net and found some clever design using power bipolar transistors and even darlingtons NPN-PNP pairs... hummm printers have that...

![](https://raw.github.com/pavelmc/carrito/master/photos/bipolar-h-bridge.jpg)

Yes, I have them PNP 2SA1568 and NPN 2SD2131, so hands to the bech... the result was a bulk & spider wired H bridge on a large heatsink from a PC power supply, it worked well but also dissipated some heat, losses was about 0.3V on PNP and 0.25V on the NPN side, that at ~700 mA, loses was about half a watt on heat, not bad in paper but the "finger test" was no so, it feels uncomfortably hot at touch.

So big and hot, yet I need a PNP transistor to control the +Vcc with the PWM of the Arduino so more bulk and heat, no thanks... I need another solution.

"Only archives a goal who seek and work for it", tells a Spanish old saying. Searching on the junk box I found a old and dust power chip with his original heatsink: 6AM14 Bingo!

That is just 3 pairs of N and P channel matched MOSFETS (7 Amps One and a half H-bridge) so I can wire the H-bridge and the remaining P-channel mosfet to control the PWM, results: under 1mA on standby and losses are under 100mW on work at 1A (no perceptible warm on full power drive), Wow MOSFET RULEZ!!!

##Too big power motor:##

![](https://raw.github.com/pavelmc/carrito/master/photos/big-motor.jpg)

I prepared the chassis to hold a power motor that is just too big to power this little monster, it eat around 1A at 8.4V and can develop a lot of speed, in the first tries I realized that and discarded it in favor of a motor half the size of that; now it eats 600mA (0.6A) at full power and plenty of speed and power for the task

##Direction Stepper motor to small##

![](https://raw.github.com/pavelmc/carrito/master/photos/small-stepper-motor.jpg)

I build the direction mechanism like an old car, moving the entire front wheels support and not each front wheel at a time in opposite direction (like any modern car). But tests showed that the selected motor (no branding or data, just a unipolar stepper motor of 200 steps per turn, see picture) was not enough for the task at least at 6-8V of power supply with current limiting.

I then moved to another stepper motor bigger and it happens to has a gear that is the same tooth pace that the main gear, this motor was from a Epson printer (is states in the chassis: EM-242) and its of bipolar type.

##Direction stepper motor controller death##

![](https://raw.github.com/pavelmc/carrito/master/photos/allegro-controller-death.jpg)

When I selected the above mentioned bipolar stepper motor I realize that I don't have a bipolar controller, then turned into the printer where it was borrowed and spotted a Allegro controller for it (A2917SEB) I get a hand saw and cut the PCB and by the datasheet I make it and get some code to get it to work.

Early test showed that the main Vcc of the motor was not enough for the task (it works on a +35V Vcc system and I have 6.0 to 8.4V from lipo batteries) I think on ways to increase the power adding more batteries up to 9.0 to 12.6V but that will nedd a BMS of three batteries and I have only 2S BMS at hand.

Testing other tricks I made a wrong connection and... it stopped working: R.I.P. A2917SEB. :(

##Direction stepper motor controller death (yes, the second one)##

Then I was forced to buy a controller, in the cuban market this is hard but fortunately I found a friend that has a A4899 Pololu controller available, just great!

Buy, mount, test: OK. It works flawlessly, on the test bench. Now it's time to put it on the real hardware... fail... what's wrong? Simple: I switched the motor power (8.4V) with the digital power (max 5.5V)... results: Pololu controller is death, you can see the burn blob on the IC.

![](https://raw.github.com/pavelmc/carrito/master/photos/pololu-controller-death.jpg)

Miraculously the Arduino Pro Mini survived!, it received the +8.4V (capable of 4.4 Amps!) and did not die... thanks to the luck I had the nRF24 module disconnected from the Arduino...

This lead to discard the mentioned bipolar motor (I'm mad at bipolar motors... GGGRRR) and switch to the actual unipolar motor with a combination of Mosfets switching the motor coils to ground and a PNP bipolar transistor as current source (1.2 A as per the motor datasheet)

The motor was again borrowed from the same printer, (MINEBEA or MNB brand, type 17PM-K303, and equivalent of a NEMA17 one) datasheet specs are 1.2A current and max 3.0V so my 6.0-8.4V is enough.

##Not enough input/output pins, the case of the 74HC595##

![](https://raw.github.com/pavelmc/carrito/master/photos/74HC595.jpg)

At some point I realized that I will have not enough input/output pins, and realizing that I have the hardware SPI bus on use I it to command a 74HC595 shift register.

Then I realize that I will need a way to move the motor pins to the shift register, there is some examples out there, even adafruit has a shield based on that idea, but I can find a lib to make this... so...

I crafted a unipolar stepper lib to address simple pins handling or returning the pin values to pass to a shift register like the 74HC595, you can find it here [stepperunipolar](http://github.com/pavelmc/stepperunipolar) it is on a dev stage, it need a lot of work on the examples and docs to get ready for the public domain, so use it at your own risk.

Still in construction, check back later.
