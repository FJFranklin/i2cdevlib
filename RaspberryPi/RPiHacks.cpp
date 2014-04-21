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

#include <unistd.h>
#include <sys/time.h>

#include "RPiHacks.h"

static struct timeval s_tval_Initial;
static bool s_bInitialized = false;

void RPiHacks::millisReset ()
{
	gettimeofday (&s_tval_Initial, 0);
	s_bInitialized = true;
}

unsigned long RPiHacks::millis () // this will work for about 24 days; don't really want to use millis at all!
{
	unsigned long ms = 0;

	if (!s_bInitialized) {
		millisReset ();
	} else {
		struct timeval tval_Current;

		gettimeofday (&tval_Current, 0);

		if (tval_Current.tv_usec < s_tval_Initial.tv_usec) {
			ms = (1000000 + tval_Current.tv_usec) - s_tval_Initial.tv_usec;
			ms = (tval_Current.tv_sec - s_tval_Initial.tv_sec - 1) * 1000 + ms / 1000;
		} else {
			ms = tval_Current.tv_usec - s_tval_Initial.tv_usec;
			ms = (tval_Current.tv_sec - s_tval_Initial.tv_sec) * 1000 + ms / 1000;
		}
	}
	return ms;
}

void RPiHacks::delay (unsigned long ms)
{
	usleep (ms * 1000);
}
