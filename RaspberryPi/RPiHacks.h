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

#ifndef RPIHACKS_HH
#define RPIHACKS_HH

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <avr/pgmspace.h>

/* Fake implementation of Arduino Serial.print/println
 */

#ifdef Serial
#undef Serial
#endif
#define Serial RPiHacks(stdout)

#ifdef HEX
#undef HEX
#endif
#define HEX true

#ifdef DEC
#undef DEC
#endif
#define DEC false

class RPiHacks
{
private:
	FILE * m_stream;

public:
	/** Class contructor: RPiHacks defines functions required to use i2cdevlib on a Raspberry Pi.
	 * 
	 * The macro "Serial" creates an unnamed instance of this class with stdout as the stream.
	 * 
	 * @param stream Stream (e.g., stdout or stderr) for output.
	 */
	RPiHacks (FILE * stream) :
	m_stream (stream)
	{
		// ...
	}

	/** Class destructor.
	 */
	~RPiHacks ()
	{
		// ...
	}

	/** Fake implementation of Arduino Serial.println().
	 * 
	 * Prints text to the output stream.
	 * 
	 * @param str Text to be printed.
	 */
	inline void print (const char * str) const {
		fputs (str, m_stream);
	}

	/** Fake implementation of Arduino Serial.print().
	 * 
	 * Prints number to the output stream.
	 * 
	 * @param number Non-negative number to be printed.
	 * @param hex    true or HEX to print as hexadecimal; false or DEC to print as decimal
	 */
	inline void print (unsigned number, bool hex) const {
		if (hex)
			fprintf (m_stream, "%x", number);
		else
			fprintf (m_stream, "%u", number);
	}

	/** Fake implementation of Arduino Serial.println().
	 * 
	 * Prints text and new line to the output stream.
	 * 
	 * @param str Text to be printed.
	 */
	inline void println (const char * str) const {
		fputs (str, m_stream);
		fputs ("\n", m_stream);
		fflush (m_stream);
	}

	/** Part of the fake implementation of Arduino millis().
	 * 
	 * @see millis()
	 * 
	 * Resets the internal timer.
	 */
	static void millisReset ();

	/** Fake implementation of Arduino millis().
	 * 
	 * Calculates number of milliseconds since last call to millisReset(); will need reset at least once every 24 days.
	 * 
	 * @see millisReset()
	 * 
	 * @return Number of milliseconds since last timer reset.
	 */
	static unsigned long millis ();

	/** Fake implementation of Arduino delay().
	 * 
	 * Puts program to sleep for specified number of milliseconds.
	 * 
	 * @param Number of milliseconds to pause for.
	 */
	static void delay (unsigned long ms);
};

/** Fake implementation of Arduino millis().
 * 
 * This is just a wrapper.
 * 
 * @see RPiHacks::millis()
 * @see RPiHacks::millisReset()
 * 
 * @return Number of milliseconds since last timer reset.
 */
static inline unsigned long millis ()
{
	return RPiHacks::millis ();
}

/** Fake implementation of Arduino delay().
 * 
 * This is just a wrapper.
 * 
 * @see RPiHacks::delay()
 * 
 * @param Number of milliseconds to pause for.
 */
static inline void delay (unsigned long ms)
{
	RPiHacks::delay (ms);
}

#endif /* ! RPIHACKS_HH */
