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

#ifndef RPI2C_HH
#define RPI2C_HH

#include <stdint.h>

#ifdef RPI2C_BUFLEN
#undef RPI2C_BUFLEN
#endif
/* This limits data reads/writes to 32 bytes; SMBus protocol and some implementations may prevent blocks larger than 32.
 * Bear in mind that 32 bytes even at 400 kHz will take nearly a millisecond to transfer.
 */
#define RPI2C_BUFLEN 32

#define RPI2C_DEFAULT_BUS "/dev/i2c-1" // default bus for newer Raspberry Pi

class RPi2c {
private:
	const char * m_error;
	int          m_fd;
	uint8_t      m_buffer[RPI2C_BUFLEN+2];

public:
	/** Set the default I2C bus.
	 * 
	 * This sets a global variable - a pointer to an instance of the RPi2c class that is subsequently returned by bus().
	 * This is useful if the program only uses one I2C bus and you don't want to pass the class instance around everywhere.
	 * 
	 * @see bus()
	 * 
	 * @param i2c Pointer to an instance of the RPi2c class; to be used as the default I2C bus.
	 */
	static void setDefaultBus (RPi2c * i2c); // short-cut to avoid having to pass the RPi2c instance around; see bus()

	/** Get the default I2C bus.
	 * 
	 * This gets the global variable previously set by a call to setDefaultBus(i2c)
	 * This is useful if the program only uses one I2C bus and you don't want to pass the class instance around everywhere.
	 * 
	 * @see setDefaultBus()
	 * 
	 * @return Pointer to the default instance of the RPi2c class.
	 */
	static RPi2c * bus (); // returns whatever was last set using setDefaultBus()

	/** Class constructor.
	 * 
	 * The constuctor does not open the bus. You should call busOpen() and then (optionally) setDefaultBus().
	 * 
	 * @see setDefaultBus()
	 * @see busOpen()
	 */
	RPi2c ();

	/** Class destructor.
	 */
	~RPi2c ();

	/** Provides a string with an error message.
	 * 
	 * The busOpen(), busRead() and busWrite() methods may fail, in which case an error message will be set with a brief explanation.
	 * 
	 * @return String with error message.
	 */
	inline const char * lastError () const { return m_error; }

	/** For 3rd parties to pass back error info.
	 * 
	 * Any message set this way needs to be a static string - it is not copied by the RPi2c class.
	 * 
	 * @param error_message If error_message is 0, the default no error is set.
	 */
	void setError (const char * error_message);

	/** Open the I2C for reading/writing.
	 * 
	 * @param bus_name The device name of the I2C bus; you can specify RPI2C_DEFAULT_BUS, which is defined as "/dev/i2c-1".
	 * 
	 * @return false on failure - use lastError() to see why.
	 */
	bool busOpen (const char * bus_name); // use this before trying to use busRead/Write

	/** Closes the I2C bus.
	 */
	void busClose ();

private:
	/** Internal method used by busRead(); responsible for actual i2c data transmission.
	 * 
	 * @return Number of bytes read; returns -1 on failure - use lastError() to see why.
	 */
	int busBRead (uint16_t device_address, uint16_t register_address, uint16_t byte_count /* max. RPI2C_BUFLEN */);
public:
	/** Read byte-data from device.
	 * 
	 * Reads 8-bit byte data from the specified device and register.
	 * 
	 * @param device_address   The 7-bit address of the i2c device (unmodified with read/write bit)
	 * @param register_address Register on device to read from
	 * @param byte_count       Number of bytes to read (maximum RPI2C_BUFLEN = 32 bytes)
	 * @param bytes            Pointer to 8-bit word data.
	 * 
	 * @return Number of bytes read; returns -1 on failure - use lastError() to see why.
	 */
	int busRead (uint16_t device_address, uint16_t register_address, uint16_t byte_count /* max. RPI2C_BUFLEN */, uint8_t * bytes);

	/** Read word-data from device.
	 * 
	 * Reads 16-bit word data from the specified device and register.
	 * 
	 * @param device_address   The 7-bit address of the i2c device (unmodified with read/write bit)
	 * @param register_address Register on device to read from
	 * @param word_count       Number of words to read (maximum RPI2C_BUFLEN/2 = 16 words)
	 * @param words            Pointer to 16-bit word data.
	 * @param data_lsb_1st     true if data is coverted to bytes least-significant-byte first
	 * 
	 * @return Number of words read; returns -1 on failure - use lastError() to see why.
	 */
	int busRead (uint16_t device_address, uint16_t register_address, uint16_t word_count /* max. RPI2C_BUFLEN/2 */, uint16_t * words,
				 bool data_lsb_1st);

private:
	/** Internal method used by busWrite(); responsible for actual i2c data transmission.
	 *
	 * @return Number of bytes written; returns -1 on failure - use lastError() to see why.
	 */
	int busBWrite (uint16_t device_address, uint16_t register_address, uint16_t byte_count /* max. RPI2C_BUFLEN */);
public:
	/** Write word-data to device.
	 * 
	 * Writes 8-bit byte data to the specified device and register.
	 * 
	 * @param device_address   The 7-bit address of the i2c device (unmodified with read/write bit)
	 * @param register_address Register on device to write to
	 * @param byte_count       Number of bytes to write (maximum RPI2C_BUFLEN = 32 bytes)
	 * @param bytes            Pointer to 8-bit byte data.
	 * 
	 * @return Number of bytes written; returns -1 on failure - use lastError() to see why.
	 */
	int busWrite (uint16_t device_address, uint16_t register_address, uint16_t byte_count /* max. RPI2C_BUFLEN */, const uint8_t * bytes);

	/** Write word-data to device.
	 * 
	 * Writes 16-bit word data to the specified device and register.
	 * 
	 * @param device_address   The 7-bit address of the i2c device (unmodified with read/write bit)
	 * @param register_address Register on device to write to
	 * @param word_count       Number of words to write (maximum RPI2C_BUFLEN/2 = 16 words)
	 * @param words            Pointer to 16-bit word data.
	 * @param data_lsb_1st     true if data is coverted to bytes least-significant-byte first
	 * 
	 * @return Number of words written; returns -1 on failure - use lastError() to see why.
	 */
	int busWrite (uint16_t device_address, uint16_t register_address, uint16_t word_count /* max. RPI2C_BUFLEN/2 */, const uint16_t * words,
				  bool data_lsb_1st);

};

#endif /* ! RPI2C_HH */
