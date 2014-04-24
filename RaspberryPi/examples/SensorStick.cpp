/* -*- mode: C++; tab-width: 4; c-basic-offset: 4; -*- */

/* ============================================
Copyright (c) 2014 Francis James Franklin

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
===============================================
*/

#include <stdio.h>
#include <math.h>
#include <unistd.h>

#include "RPi2c.h"

#include "ADXL345/ADXL345.h"
#include "HMC5883L/HMC5883L.h"
#include "ITG3200/ITG3200.h"

int main ()
{
	RPi2c i2c;

	if (!i2c.busOpen (RPI2C_DEFAULT_BUS)) {
		fprintf (stderr, "SensorStick: Failed to open bus '%s', because:\n    %s\n", RPI2C_DEFAULT_BUS, i2c.lastError ());
		return -1;
	}

	RPi2c::setDefaultBus (&i2c);

	ADXL345 accel;
    accel.initialize ();

	ITG3200 gyro;
    gyro.initialize ();

	HMC5883L mag;
    mag.initialize ();

	for (int i = 0; i < 10; i++) {
		int16_t ax, ay, az;
		accel.getAcceleration (&ax, &ay, &az);
		fprintf (stdout, "ADXL345: ax=%d ay=%d az=%d\n", (int) ax, (int) ay, (int) az);

		int16_t gx, gy, gz;
		gyro.getRotation (&gx, &gy, &gz);
		fprintf (stdout, "ITG-3200: gx=%d gy=%d gz=%d\n", (int) gx, (int) gy, (int) gz);

		int16_t mx, my, mz;
		mag.getHeading (&mx, &my, &mz);

		float heading = atan2 (my, mx);
		if (heading < 0) {
			heading += 2 * M_PI;
		}
		fprintf (stdout, "HMC5883L: mx=%d my=%d mz=%d, heading[0=North]=%f\n", (int) mx, (int) my, (int) mz, heading * 180/M_PI);

		usleep (100000); // 0.1s
	}
	for (int i = 0; i < 1000; i++) {
		int16_t A[3];

		int status = i2c.busRead (ADXL345_ADDRESS_ALT_LOW, ADXL345_RA_DATAX0, 3, reinterpret_cast<uint16_t *>(A), true);
		if (status < 0) {
			fprintf (stderr, "SensorStick:  %s\n", i2c.lastError ());
			break;
		}

		int16_t G[3];

		status = i2c.busRead (ITG3200_ADDRESS_AD0_LOW, ITG3200_RA_GYRO_XOUT_H, 3, reinterpret_cast<uint16_t *>(G), false);
		if (status < 0) {
			fprintf (stderr, "SensorStick:  %s\n", i2c.lastError ());
			break;
		}

		// The magnetometer is a little trickier, so skipping here...

		fprintf (stdout, "A = (%6d, %6d, %6d), G = (%6d, %6d, %6d)\r",
				 (int) A[0], (int) A[1], (int) A[2],
				 (int) G[0], (int) G[1], (int) G[2]);
		fflush (stdout);

		usleep (10000); // 0.01s
	}
	fprintf (stdout, "\n");

	return 0;
}
