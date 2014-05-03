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
#include <signal.h>

#include "RPi2c.h"
#include "RPiHacks.h"

#include "ADXL345/ADXL345.h"
#include "HMC5883L/HMC5883L.h"
#include "ITG3200/ITG3200.h"

struct ag_stats {
	struct {
		struct {
			int16_t ag_min;
			int16_t ag_max;
			int16_t ag_mean;
		} x, y, z;
	} acc, gyr;

	long count;
};

static struct {
	FILE * out;
} global_vars;

void end_measurement (int signum)
{
	if (global_vars.out) {
		fprintf (global_vars.out, "\nSignal %d received. Quitting...\n");
		fclose (global_vars.out);
	}
	_exit (0);
}

void accel_awake (ADXL345 & accel)
{
	accel.setSleepEnabled (false /* wake up! */);
	accel.setMeasureEnabled (false /* enter stand-by mode */);
	usleep (1000);
	accel.setMeasureEnabled (true /* come out of stand-by mode */);
	accel.setRate (0x0E); // 1600Hz sample rate (800Hz bandwidth)
	accel.setLowPowerEnabled (false /* disabled */);
	accel.setOffset (0, 0, 0);
	accel.setFullResolution (0x0A /* uint8_t resolution - should be a bit, not a byte! */); // 13-bit mode
}

void accel_sleep (ADXL345 & accel)
{
	accel.setRate (0x0A); // 100Hz sample rate (50Hz bandwidth)
	accel.setLowPowerEnabled (true /* enabled */);
	accel.setMeasureEnabled (false /* enter stand-by mode */);
	accel.setSleepEnabled (true /* go to sleep */);
}

void get_stats (struct ag_stats & as, RPi2c & i2c, bool bQuiet = false)
{
	for (int s = 10; s > 0; s--) {
		if (!bQuiet) {
			fprintf (stdout, "%d", s);
			fflush (stdout);
		}
		usleep (100000); // 0.1s

		for (int ds = 1; ds < 10; ds++) {
			if (!bQuiet) {
				fputs (".", stdout);
				fflush (stdout);
			}
			usleep (100000); // 0.1s
		}
	}
	if (!bQuiet) {
		fputs ("0\n", stdout);
		fflush (stdout);
	}

	RPiHacks::millisReset ();

	as.count = 0;
	
	long acc_x_sum = 0;
	long acc_y_sum = 0;
	long acc_z_sum = 0;

	long gyr_x_sum = 0;
	long gyr_y_sum = 0;
	long gyr_z_sum = 0;

	while (RPiHacks::millis () < 1000) {
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

		if (!as.count) {
			as.acc.x.ag_min = A[0];
			as.acc.x.ag_max = A[0];
			as.acc.y.ag_min = A[1];
			as.acc.y.ag_max = A[1];
			as.acc.z.ag_min = A[2];
			as.acc.z.ag_max = A[2];

			as.gyr.x.ag_min = G[0];
			as.gyr.x.ag_max = G[0];
			as.gyr.y.ag_min = G[1];
			as.gyr.y.ag_max = G[1];
			as.gyr.z.ag_min = G[2];
			as.gyr.z.ag_max = G[2];
		} else {
			if (as.acc.x.ag_min > A[0])
				as.acc.x.ag_min = A[0];
			if (as.acc.x.ag_max < A[0])
				as.acc.x.ag_max = A[0];
			if (as.acc.y.ag_min > A[1])
				as.acc.y.ag_min = A[1];
			if (as.acc.y.ag_max < A[1])
				as.acc.y.ag_max = A[1];
			if (as.acc.z.ag_min > A[2])
				as.acc.z.ag_min = A[2];
			if (as.acc.z.ag_max < A[2])
				as.acc.z.ag_max = A[2];

			if (as.gyr.x.ag_min > G[0])
				as.gyr.x.ag_min = G[0];
			if (as.gyr.x.ag_max < G[0])
				as.gyr.x.ag_max = G[0];
			if (as.gyr.y.ag_min > G[1])
				as.gyr.y.ag_min = G[1];
			if (as.gyr.y.ag_max < G[1])
				as.gyr.y.ag_max = G[1];
			if (as.gyr.z.ag_min > G[2])
				as.gyr.z.ag_min = G[2];
			if (as.gyr.z.ag_max < G[2])
				as.gyr.z.ag_max = G[2];
		}

		acc_x_sum += A[0];
		acc_y_sum += A[1];
		acc_z_sum += A[2];

		gyr_x_sum += G[0];
		gyr_y_sum += G[1];
		gyr_z_sum += G[2];

		++as.count;
	}
	if (as.count) {
		as.acc.x.ag_mean = acc_x_sum / as.count;
		as.acc.y.ag_mean = acc_y_sum / as.count;
		as.acc.z.ag_mean = acc_z_sum / as.count;

		as.gyr.x.ag_mean = gyr_x_sum / as.count;
		as.gyr.y.ag_mean = gyr_y_sum / as.count;
		as.gyr.z.ag_mean = gyr_z_sum / as.count;
	}
}

void print_stats (struct ag_stats & as)
{
	fprintf (stdout, "%lu samples read in 10s\n", as.count);
	fprintf (stdout, "acc-x: %d < %d < %d\n", (int) as.acc.x.ag_min, (int) as.acc.x.ag_mean, (int) as.acc.x.ag_max);
	fprintf (stdout, "acc-y: %d < %d < %d\n", (int) as.acc.y.ag_min, (int) as.acc.y.ag_mean, (int) as.acc.y.ag_max);
	fprintf (stdout, "acc-z: %d < %d < %d\n", (int) as.acc.z.ag_min, (int) as.acc.z.ag_mean, (int) as.acc.z.ag_max);
	fprintf (stdout, "gyr-x: %d < %d < %d\n", (int) as.gyr.x.ag_min, (int) as.gyr.x.ag_mean, (int) as.gyr.x.ag_max);
	fprintf (stdout, "gyr-y: %d < %d < %d\n", (int) as.gyr.y.ag_min, (int) as.gyr.y.ag_mean, (int) as.gyr.y.ag_max);
	fprintf (stdout, "gyr-z: %d < %d < %d\n", (int) as.gyr.z.ag_min, (int) as.gyr.z.ag_mean, (int) as.gyr.z.ag_max);
	fflush (stdout);
}

int main (int argc, char ** argv)
{
	bool bSleep = false;
	bool bBasicTest = true;
	bool bCalibration = false;
	bool bMeasurement = false;

	const char * filename = 0;

	int oz = 0;
	int ox = 0;
	int oy = 0;

	if (argc > 1) {
		if (strcmp (argv[1],"--sleep") == 0) {
			bBasicTest = false;
			bSleep = true;
		}
		if (strcmp (argv[1],"--calibration") == 0) {
			bBasicTest = false;
			bCalibration = true;
		}
		if (strcmp (argv[1],"--measurement") == 0) {
			bBasicTest = false;
			bMeasurement = true;
		}
	}
	if (bMeasurement) {
		for (int argi = 2; argi < argc; argi++) {
			if (strncmp (argv[argi], "ox=", 3) == 0) {
				if (sscanf (argv[argi]+3, "%d", &ox) != 1) {
					fprintf (stderr, "error in argument \"%s\"!\n", argv[argi]);
				}
			}
			if (strncmp (argv[argi], "oy=", 3) == 0) {
				if (sscanf (argv[argi]+3, "%d", &oy) != 1) {
					fprintf (stderr, "error in argument \"%s\"!\n", argv[argi]);
				}
			}
			if (strncmp (argv[argi], "oz=", 3) == 0) {
				if (sscanf (argv[argi]+3, "%d", &oz) != 1) {
					fprintf (stderr, "error in argument \"%s\"!\n", argv[argi]);
				}
			}
			if (strncmp (argv[argi], "--out=", 6) == 0) {
				filename = argv[argi] + 6;
			}
		}
		if (!filename) {
			fprintf (stderr, "error: no output filename specified!\n");
			return 0;
		}
		fprintf (stdout, "Measuring: ox=%d, oy=%d, oz=%d, output filename = \"%s\"\n", ox, oy, oz, filename);
	}

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

	gyro.setDLPFBandwidth (0); // 8kHz internal sampling; low-pass @ 256Hz
	gyro.setRate (4); // sample rate should be 2kHz

	if (bSleep) {
		accel_sleep (accel);
		return 0;
	}

	if (bBasicTest) {
		accel_awake (accel);
		accel.setOffset (0, 0, 0);

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
		return (0);
	}

	if (bCalibration) {
		accel_awake (accel);
		accel.setOffset (0, 0, 0);

		struct ag_stats ag1;
		fprintf (stdout, "\n* * * Face 1 [Z] Up:\n");
		get_stats (ag1, i2c);
		print_stats (ag1);
	
		struct ag_stats ag2;
		fprintf (stdout, "\n* * * Face 1 [Z] Down:\n");
		get_stats (ag2, i2c);
		print_stats (ag2);
	
		struct ag_stats ag3;
		fprintf (stdout, "\n* * * Face 2 [X] Up:\n");
		get_stats (ag3, i2c);
		print_stats (ag3);
	
		struct ag_stats ag4;
		fprintf (stdout, "\n* * * Face 2 [X] Down:\n");
		get_stats (ag4, i2c);
		print_stats (ag4);
	
		struct ag_stats ag5;
		fprintf (stdout, "\n* * * Face 3 [Y] Up:\n");
		get_stats (ag5, i2c);
		print_stats (ag5);
	
		struct ag_stats ag6;
		fprintf (stdout, "\n* * * Face 3 [Y] Down:\n");
		get_stats (ag6, i2c);
		print_stats (ag6);

		oz = (ag1.acc.z.ag_mean + ag2.acc.z.ag_mean) / 2;
		ox = (ag3.acc.x.ag_mean + ag4.acc.x.ag_mean) / 2;
		oy = (ag5.acc.y.ag_mean + ag6.acc.y.ag_mean) / 2;

		fprintf (stdout, "\n==== Origin: ox=%d, oy=%d, oz=%d ====\n", ox, oy, oz);

		accel.setOffset ((int8_t) -ox/4, (int8_t) -oy/4, (int8_t) -oz/4);

		float gx = ((float) ag6.acc.x.ag_mean) - ((float) ox);
		float gy = ((float) ag6.acc.y.ag_mean) - ((float) oy);
		float gz = ((float) ag6.acc.z.ag_mean) - ((float) oz);
		float gr = sqrt (gx * gx + gy * gy + gz * gz);

		fprintf (stdout, "\nLast gravity measurement was |(%f,%f,%f)|=%f\n", gx, gy, gz, gr);

		struct ag_stats agv;
		fprintf (stdout, "\n* * * Verifying:\n");
		get_stats (agv, i2c);
		print_stats (agv);
	
		accel_sleep (accel);

		return 0;
	}

	if (bMeasurement) {
		global_vars.out = fopen (filename, "w");
		if (!global_vars.out) {
			fprintf (stderr, "unable to write to file \"%s\"!\n", filename);
			return 0;
		}

		struct sigaction action;
		action.sa_handler = end_measurement;
		sigemptyset (&action.sa_mask);
		action.sa_flags = 0;
		sigaction (SIGHUP, &action, 0);
		sigaction (SIGINT, &action, 0);

		accel_awake (accel);

		// establish baseline
		struct ag_stats agv;
		// fprintf (stdout, "\n* * * Getting Baseline:\n");
		get_stats (agv, i2c, true);
		// fprintf (stdout, "\n* * * Done. Measuring now:\n");

		fprintf (global_vars.out, "ADXL345,x\ty\tz\tITG-3200,x\ty=\tz\t[ms]\n");
		fprintf (global_vars.out, "Rows 3-5:\tmean\tmin\tmax\tcount(1s)=\t%ld\t\n", agv.count);
		fprintf (global_vars.out, "%d\t%d\t%d\t%d\t%d\t%d\t%lu\n",
				 ((int) agv.acc.x.ag_mean) - ox, ((int) agv.acc.y.ag_mean) - oy, ((int) agv.acc.z.ag_mean) - oz,
				 (int) agv.gyr.x.ag_mean, (int) agv.gyr.y.ag_mean, (int) agv.gyr.z.ag_mean, RPiHacks::millis ());
		fprintf (global_vars.out, "%d\t%d\t%d\t%d\t%d\t%d\t\n",
				 ((int) agv.acc.x.ag_min) - ox, ((int) agv.acc.y.ag_min) - oy, ((int) agv.acc.z.ag_min) - oz,
				 (int) agv.gyr.x.ag_min, (int) agv.gyr.y.ag_min, (int) agv.gyr.z.ag_min);
		fprintf (global_vars.out, "%d\t%d\t%d\t%d\t%d\t%d\t\n",
				 ((int) agv.acc.x.ag_max) - ox, ((int) agv.acc.y.ag_max) - oy, ((int) agv.acc.z.ag_max) - oz,
				 (int) agv.gyr.x.ag_max, (int) agv.gyr.y.ag_max, (int) agv.gyr.z.ag_max);

		while (true) {
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

			fprintf (global_vars.out, "%d\t%d\t%d\t%d\t%d\t%d\t%lu\n",
					 ((int) A[0]) - ((int) agv.acc.x.ag_mean),
					 ((int) A[1]) - ((int) agv.acc.y.ag_mean),
					 ((int) A[2]) - ((int) agv.acc.z.ag_mean),
					 ((int) G[0]) - ((int) agv.gyr.x.ag_mean),
					 ((int) G[1]) - ((int) agv.gyr.y.ag_mean),
					 ((int) G[2]) - ((int) agv.gyr.z.ag_mean),
					 RPiHacks::millis ());
			fflush (global_vars.out);
		}
		_exit (0); // unexpected
	}
}
