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
#include <sys/time.h>

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

static struct timeval s_tval_Initial;

static void timerStart ()
{
	gettimeofday (&s_tval_Initial, 0);
}

static unsigned long timerStop () // this will work for about 24 days; don't really want to use millis at all!
{
	unsigned long us = 0;

	struct timeval tval_Current;

	gettimeofday (&tval_Current, 0);

	if (tval_Current.tv_usec < s_tval_Initial.tv_usec) {
		us = (1000000 + tval_Current.tv_usec) - s_tval_Initial.tv_usec;
		us = (tval_Current.tv_sec - s_tval_Initial.tv_sec - 1) * 1000000 + us;
	} else {
		us = tval_Current.tv_usec - s_tval_Initial.tv_usec;
		us = (tval_Current.tv_sec - s_tval_Initial.tv_sec) * 1000000 + us;
	}
	return us;
}

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
	m_fd (-1),
	m_transferTime(0),
	m_bTransferTime(false),
	m_bEnableRS(true),
	m_bSpecifyRegister(true)
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

	if (m_bSpecifyRegister) {
		if (register_address & 0xFF00) {
			m_buffer[0] = static_cast<uint8_t>((register_address >> 8) & 0x00FF);
		} else {
			++register_byte_ptr;
			--register_byte_count;
		}
		m_buffer[1] = static_cast<uint8_t>(register_address & 0x00FF);
	}

	struct i2c_msg message[2] = {
		{ device_address, 0, register_byte_count, reinterpret_cast<char *>(register_byte_ptr) },
		{ device_address, I2C_M_RD, byte_count, reinterpret_cast<char *>(m_buffer + 2) }
	};

	int status = 0;

	if (m_bEnableRS && m_bSpecifyRegister) {
		struct i2c_rdwr_ioctl_data data = { message, 2 };
		status = ioctl (m_fd, I2C_RDWR, &data);
	} else {
		if (m_bSpecifyRegister) {
			struct i2c_rdwr_ioctl_data data = { message, 1 };
			status = ioctl (m_fd, I2C_RDWR, &data);
		}
		if (status >= 0) {
			struct i2c_rdwr_ioctl_data data = { message + 1, 1 };
			status = ioctl (m_fd, I2C_RDWR, &data);
		}
	}

	if (status < 0) {
		m_error = s_error_rderr;
		return -1;
	}
	return byte_count;
}

/* Returns number of bytes read; returns -1 on failure - use last_error() to see why.
 */
int RPi2c::busRead (uint16_t device_address, uint16_t register_address, uint16_t byte_count, uint8_t * bytes, bool bSpecifyRegister)
{
	if (m_bTransferTime) {
		m_transferTime = 0;
		timerStart ();
	}

	m_bSpecifyRegister = bSpecifyRegister;

	m_error = s_error_none;

	if (byte_count && !bytes) {
		m_error = s_error_rddata;
		return -1;
	}

	int status = 0;

	uint16_t byte_count_total = 0;

	while (byte_count > 0) {
		uint16_t byte_count_this = (byte_count > RPI2C_BUFLEN) ? RPI2C_BUFLEN : byte_count;

		status = busBRead (device_address, register_address, byte_count);

		if (status < 0) break;

		memcpy (bytes, m_buffer + 2, byte_count);

		// logically, this is the only place to add a time-out // TODO ??

		byte_count_total += byte_count_this;
		byte_count -= byte_count_this;
		bytes += byte_count_this;

		m_bSpecifyRegister = false;
	}

	if (m_bTransferTime) {
		m_transferTime = timerStop ();
	}
	return (status < 0) ? status : byte_count_total;
}

/* Returns number of words read; returns -1 on failure - use last_error() to see why.
 */
int RPi2c::busRead (uint16_t device_address, uint16_t register_address, uint16_t word_count, uint16_t * words, bool data_lsb_1st, bool bSpecifyRegister)
{
	if (m_bTransferTime) {
		m_transferTime = 0;
		timerStart ();
	}

	m_bSpecifyRegister = bSpecifyRegister;

	m_error = s_error_none;

	if (word_count && !words) {
		m_error = s_error_rddata;
		return -1;
	}

	int status = 0;

	uint16_t word_count_total = 0;

	while (word_count > 0) {
		uint16_t word_count_this = (2 * word_count > RPI2C_BUFLEN) ? (RPI2C_BUFLEN / 2) : word_count;
		uint16_t byte_count = word_count * 2;

		uint8_t * byte = m_buffer + 2;

		status = busBRead (device_address, register_address, byte_count);

		if (status < 0) break;

		for (int iw = 0; iw < word_count_this; iw++) {
			if (data_lsb_1st) {
				words[iw]  = static_cast<uint16_t>(*byte++);
				words[iw] |= static_cast<uint16_t>(*byte++) << 8;
			} else {
				words[iw]  = static_cast<uint16_t>(*byte++) << 8;
				words[iw] |= static_cast<uint16_t>(*byte++);
			}
		}

		// logically, this is the only place to add a time-out // TODO ??

		word_count_total += word_count_this;
		word_count -= word_count_this;
		words += word_count_this;

		m_bSpecifyRegister = false;
	}

	if (m_bTransferTime) {
		m_transferTime = timerStop ();
	}
	return (status < 0) ? status : word_count_total;
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

	uint8_t * register_byte_ptr = m_buffer + 2;
	uint16_t register_byte_count = 0;

	if (m_bSpecifyRegister) {
		*--register_byte_ptr = static_cast<uint8_t>(register_address & 0x00FF);
		++register_byte_count;

		if (register_address & 0xFF00) {
			*--register_byte_ptr = static_cast<uint8_t>((register_address >> 8) & 0x00FF);
			++register_byte_count;
		}
	}
	struct i2c_msg message[1] = {
		{ device_address, 0, register_byte_count + byte_count, reinterpret_cast<char *>(register_byte_ptr) }
	};
	struct i2c_rdwr_ioctl_data data = { message, 1 };

	int status = ioctl (m_fd, I2C_RDWR, &data);

	if (status < 0) {
		m_error = s_error_wrerr;
		return -1;
	}
	return byte_count;
}

/* Returns number of bytes written; returns -1 on failure - use last_error() to see why.
 */
int RPi2c::busWrite (uint16_t device_address, uint16_t register_address, uint16_t byte_count, const uint8_t * bytes, bool bSpecifyRegister)
{
	if (m_bTransferTime) {
		m_transferTime = 0;
		timerStart ();
	}

	m_bSpecifyRegister = bSpecifyRegister;

	m_error = s_error_none;

	if (byte_count && !bytes) {
		m_error = s_error_wrdata;
		return -1;
	}

	int status = 0;

	uint16_t byte_count_total = 0;

	while (byte_count > 0) {
		uint16_t byte_count_this = (byte_count > RPI2C_BUFLEN) ? RPI2C_BUFLEN : byte_count;

		memcpy (m_buffer + 2, bytes, byte_count_this);

		status = busBWrite (device_address, register_address, byte_count_this);

		if (status < 0) break;

		// logically, this is the only place to add a time-out // TODO ??

		byte_count_total += byte_count_this;
		byte_count -= byte_count_this;
		bytes += byte_count_this;

		m_bSpecifyRegister = false;
	}

	if (m_bTransferTime) {
		m_transferTime = timerStop ();
	}
	return (status < 0) ? status : byte_count_total;
}

/* Returns number of words written; returns -1 on failure - use last_error() to see why.
 */
int RPi2c::busWrite (uint16_t device_address, uint16_t register_address, uint16_t word_count, const uint16_t * words, bool data_lsb_1st, bool bSpecifyRegister)
{
	if (m_bTransferTime) {
		m_transferTime = 0;
		timerStart ();
	}

	m_bSpecifyRegister = bSpecifyRegister;

	m_error = s_error_none;

	if (word_count && !words) {
		m_error = s_error_wrdata;
		return -1;
	}

	int status = 0;

	uint16_t word_count_total = 0;

	while (word_count > 0) {
		uint16_t word_count_this = (2 * word_count > RPI2C_BUFLEN) ? (RPI2C_BUFLEN / 2) : word_count;
		uint16_t byte_count = word_count * 2;

		uint8_t * byte = m_buffer + 2;

		for (int iw = 0; iw < word_count_this; iw++) {
			if (data_lsb_1st) {
				*byte++ = static_cast<uint8_t>(words[iw] & 0xFF);
				*byte++ = static_cast<uint8_t>((words[iw] >> 8) & 0xFF);
			} else {
				*byte++ = static_cast<uint8_t>((words[iw] >> 8) & 0xFF);
				*byte++ = static_cast<uint8_t>(words[iw] & 0xFF);
			}
		}

		status = busBWrite (device_address, register_address, byte_count);

		if (status < 0) break;

		// logically, this is the only place to add a time-out // TODO ??

		word_count_total += word_count_this;
		word_count -= word_count_this;
		words += word_count_this;

		m_bSpecifyRegister = false;
	}

	if (m_bTransferTime) {
		m_transferTime = timerStop ();
	}
	return (status < 0) ? status : word_count_total;
}
