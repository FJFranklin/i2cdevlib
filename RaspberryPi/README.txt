This is my port of I2Cdevlib for the Raspberry Pi; my latest version is here:
https://github.com/FJFranklin/i2cdevlib

It seems to be working, but I haven't tested extensively. I have a SensorStick which I have only just started playing with.

I have tried for minimal intrusion into the Arduino directory, and there I have kept the changes in the I2Cdev class itself, except IAQ2000 had its own sub-implementation of I2Cdev which I have moved into the main I2Cdev class and rewritten but not tested.

The Makefile is a little clunky, but allows everything to be built in a parallel directory, e.g., "i2cdevlib/build" with:

make -f ../RaspberryPi/Makefile

In the RaspberryPi directory there is
 - a standalone class implementing the core I2C stuff (RPi2c),
 - a class called RPiHacks which defines miscellaneous functions needed to make i2cdevlib build on the Raspberry Pi,
 - a sub-directory called "examples" which has the SensorStick code, which is very basic at the moment.

I am not an I2C expert so I'm still very uncertain about device support...

Francis James Franklin
https://blogs.ncl.ac.uk/francisfranklin/
