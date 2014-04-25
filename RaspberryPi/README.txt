This is my port of I2Cdevlib for the Raspberry Pi; my latest version is here:
https://github.com/FJFranklin/i2cdevlib

It seems to be working, but I haven't tested extensively. I have a SensorStick which I have only just started playing with.

I have tried for minimal intrusion into the Arduino directory, and there I have kept the changes in the I2Cdev class itself.

Device issues:
 - The DS1307 and the SSD1308 both use Arduino PROGMEM stuff, which I'm not familiar with.
 - The IAQ2000 has its own sub-implementation of I2Cdevlib which I want to move into the main I2Cdevlib class.

The Makefile is a little clunky, but allows everything to be built in a parallel directory, e.g., "i2cdevlib/build" with:

make -f ../RaspberryPi/Makefile

In the RaspberryPi directory there is
 - a standalone class implementing the core I2C stuff (RPi2c),
 - a class called RPiHacks which defines miscellaneous functions needed to make i2cdevlib build on the Raspberry Pi,
 - a sub-directory called "kernel" which should be ignored, was added by accident and will probably be deleted in the near future, and
 - a sub-directory called "examples" which has the SensorStick code, which is very basic at the moment.
I am not an I2C expert so I'm still very uncertain about device support...

Francis James Franklin
https://blogs.ncl.ac.uk/francisfranklin/
