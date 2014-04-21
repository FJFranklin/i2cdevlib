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

// #include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <linux/i2c-dev.h>

#include "RPi2c.h"

static const char * s_error_none    = "(none)";
static const char * s_error_reopen  = "RPi2c::bus_open: error: The I2C bus already open.";
static const char * s_error_busname = "RPi2c::bus_open: error: Invalid bus name.";
static const char * s_error_noopen  = "RPi2c::bus_open: error: Unable to open I2C bus.";
static const char * s_error_rddata  = "RPi2c::bus_read: error: Invalid pointer to data.";
static const char * s_error_nobusr  = "RPi2c::bus_bread: error: No open I2C bus.";
static const char * s_error_slaver  = "RPi2c::bus_bread: error: Failed to put device into slave mode.";
static const char * s_error_rderr   = "RPi2c::bus_bread: error: Failed to transfer data.";
static const char * s_error_wrdata  = "RPi2c::bus_write: error: Invalid pointer to data.";
static const char * s_error_nobusw  = "RPi2c::bus_bwrite: error: No open I2C bus.";
static const char * s_error_slavew  = "RPi2c::bus_bwrite: error: Failed to put device into slave mode.";
static const char * s_error_wrerr   = "RPi2c::bus_bwrite: error: Failed to transfer data.";

static RPi2c * s_default_bus = 0;

void RPi2c::setDefaultBus (RPi2c * i2c)
{
	s_default_bus = i2c;
}

RPi2c * RPi2c::bus ()
{
	return s_default_bus;
}

void RPi2c::setError (const char * error_message)
{
	m_error = error_message ? error_message : s_error_none;
}

RPi2c::RPi2c () :
	m_error(s_error_none),
	m_fd (-1)
{
	//
}

RPi2c::~RPi2c ()
{
	busClose ();
}

bool RPi2c::busOpen (const char * bus_name)
{
	m_error = s_error_none;

	if (m_fd >= 0) {
		m_error = s_error_reopen;
		return false;
	}

	if (!bus_name) {
		m_error = s_error_busname;
		return false;
	}

	m_fd = open (bus_name, O_RDWR);
	if (m_fd < 0) {
		m_error = s_error_noopen;
		return false;
	}

	return true;
}

void RPi2c::busClose ()
{
	m_error = s_error_none;

	if (m_fd >= 0) {
		close (m_fd);
		m_fd = -1;
	}
}

/* Returns number of bytes read; returns -1 on failure - use last_error() to see why.
 */
int RPi2c::busBRead (uint16_t device_address, uint16_t register_address, uint16_t byte_count)
{
	if (m_fd < 0) {
		m_error = s_error_nobusr;
		return -1;
	}

	if (ioctl (m_fd, I2C_SLAVE, device_address) < 0) {
		m_error = s_error_slaver;
		return -1;
	}

	uint8_t * register_byte_ptr = m_buffer;
	uint16_t register_byte_count = 2;

	if (register_address & 0xFF00) {
		m_buffer[0] = static_cast<uint8_t>((register_address >> 8) & 0x00FF);
	} else {
		++register_byte_ptr;
		--register_byte_count;
	}
	m_buffer[1] = static_cast<uint8_t>(register_address & 0x00FF);

	struct i2c_msg message[2] = {
		{ device_address, 0, register_byte_count, reinterpret_cast<char *>(register_byte_ptr) },
		{ device_address, I2C_M_RD, byte_count, reinterpret_cast<char *>(m_buffer + 2) }
	};
	struct i2c_rdwr_ioctl_data data = { message, 2 };

	if (ioctl (m_fd, I2C_RDWR, &data) < 0) {
		m_error = s_error_rderr;
		return -1;
	}
	return byte_count;
}

/* Returns number of bytes read; returns -1 on failure - use last_error() to see why.
 */
int RPi2c::busRead (uint16_t device_address, uint16_t register_address, uint16_t byte_count, uint8_t * bytes)
{
	m_error = s_error_none;

	if (byte_count < 1) return 0; // ?? Are there special cases? What about reading without specifying a register? TODO: Check!

	if (!bytes) {
		m_error = s_error_rddata;
		return -1;
	}

	if (byte_count > RPI2C_BUFLEN) // unclear whether this is strictly correct / necessary TODO: Check!
		byte_count = RPI2C_BUFLEN;

	int status = busBRead (device_address, register_address, byte_count);

	if (status > 0) {
		memcpy (bytes, m_buffer + 2, byte_count);
	}
	return status;
}

/* Returns number of words read; returns -1 on failure - use last_error() to see why.
 */
int RPi2c::busRead (uint16_t device_address, uint16_t register_address, uint16_t word_count, uint16_t * words, bool data_lsb_1st)
{
	m_error = s_error_none;

	if (word_count < 1) return 0; // ?? Are there special cases? What about reading without specifying a register? TODO: Check!

	if (!words) {
		m_error = s_error_rddata;
		return -1;
	}

	uint16_t byte_count = word_count * 2;

	if (byte_count > RPI2C_BUFLEN) // unclear whether this is strictly correct / necessary TODO: Check!
		byte_count = RPI2C_BUFLEN;

	int status = busBRead (device_address, register_address, byte_count);

	if (status > 0) {
		word_count = byte_count / 2;
		
		uint8_t * byte = m_buffer + 2;

		for (int iw = 0; iw < word_count; iw++) {
			if (data_lsb_1st) {
				words[iw]  = static_cast<uint16_t>(*byte++);
				words[iw] |= static_cast<uint16_t>(*byte++) << 8;
			} else {
				words[iw]  = static_cast<uint16_t>(*byte++) << 8;
				words[iw] |= static_cast<uint16_t>(*byte++);
			}
		}
		status = word_count;
	}
	return status;
}

/* Returns number of bytes written; returns -1 on failure - use last_error() to see why.
 */
int RPi2c::busBWrite (uint16_t device_address, uint16_t register_address, uint16_t byte_count)
{
	if (m_fd < 0) {
		m_error = s_error_nobusw;
		return -1;
	}

	if (ioctl (m_fd, I2C_SLAVE, device_address) < 0) {
		m_error = s_error_slavew;
		return -1;
	}

	uint8_t * register_byte_ptr = m_buffer;
	uint16_t register_byte_count = 2;

	if (register_address & 0xFF00) {
		m_buffer[0] = static_cast<uint8_t>((register_address >> 8) & 0x00FF);
	} else {
		++register_byte_ptr;
		--register_byte_count;
	}
	m_buffer[1] = static_cast<uint8_t>(register_address & 0x00FF);

	struct i2c_msg message[1] = {
		{ device_address, 0, register_byte_count + byte_count, reinterpret_cast<char *>(register_byte_ptr) }
	};
	struct i2c_rdwr_ioctl_data data = { message, 1 };

	if (ioctl (m_fd, I2C_RDWR, &data) < 0) {
		m_error = s_error_wrerr;
		return -1;
	}
	return byte_count;
}

/* Returns number of bytes written; returns -1 on failure - use last_error() to see why.
 */
int RPi2c::busWrite (uint16_t device_address, uint16_t register_address, uint16_t byte_count, const uint8_t * bytes)
{
	m_error = s_error_none;

	if (byte_count < 1) return 0; // ?? Are there special cases? What about writing without specifying a register? TODO: Check!

	if (!bytes) {
		m_error = s_error_wrdata;
		return -1;
	}

	if (byte_count > RPI2C_BUFLEN)
		byte_count = RPI2C_BUFLEN;

	memcpy (m_buffer + 2, bytes, byte_count);

	return busBWrite (device_address, register_address, byte_count);
}

/* Returns number of words written; returns -1 on failure - use last_error() to see why.
 */
int RPi2c::busWrite (uint16_t device_address, uint16_t register_address, uint16_t word_count, const uint16_t * words, bool data_lsb_1st)
{
	m_error = s_error_none;

	if (word_count < 1) return 0; // ?? Are there special cases? What about writing without specifying a register? TODO: Check!

	if (!words) {
		m_error = s_error_wrdata;
		return -1;
	}

	uint16_t byte_count = word_count * 2;

	if (byte_count > RPI2C_BUFLEN)
		byte_count = RPI2C_BUFLEN;

	word_count = byte_count / 2;

	uint8_t * byte = m_buffer + 2;

	for (int iw = 0; iw < word_count; iw++) {
		if (data_lsb_1st) {
			*byte++ = static_cast<uint8_t>(words[iw] & 0xFF);
			*byte++ = static_cast<uint8_t>((words[iw] >> 8) & 0xFF);
		} else {
			*byte++ = static_cast<uint8_t>((words[iw] >> 8) & 0xFF);
			*byte++ = static_cast<uint8_t>(words[iw] & 0xFF);
		}
	}

	int status = busBWrite (device_address, register_address, byte_count);

	return (status > 0) ? word_count : status;
}
