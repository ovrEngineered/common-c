/**
 * Copyright 2013 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "cxa_posix_usart.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdbool.h>
#include <cxa_assert.h>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static bool set_interface_attribs (int fd, int speed, int parity);
static bool set_blocking (int fd, int should_block);


// ********  local variable declarations *********


// ******** global function implementations ********
bool cxa_posix_usart_init_noHH(cxa_usart_t *const usartIn, char *const pathIn, const int baudRateIn)
{
	cxa_assert(usartIn);
	cxa_assert(pathIn);

	usartIn->fd = open(pathIn, O_RDWR | O_NOCTTY | O_SYNC);
	if( usartIn->fd < 0 ) return false;

	if( !set_interface_attribs (usartIn->fd, baudRateIn, 0) ) return false;
	if( !set_blocking (usartIn->fd, 0) ) return false;

	return true;
}


void cxa_posix_usart_close(cxa_usart_t *const usartIn)
{
	cxa_assert(usartIn);

	close(usartIn->fd);
}


bool cxa_usart_read(cxa_usart_t* usartIn, void* buffIn, size_t desiredReadSize_bytesIn, size_t* actualReadSize_bytesOut)
{
	cxa_assert(usartIn);

	// perform our read and check the return value
	ssize_t retVal_read = read(usartIn->fd, buffIn, desiredReadSize_bytesIn);
	if( retVal_read < 0 ) return false;

	// set our output parameter if asked
	if( actualReadSize_bytesOut != NULL ) *actualReadSize_bytesOut = (size_t)retVal_read;

	return true;
}


bool cxa_usart_write(cxa_usart_t* usartIn, void* buffIn, size_t bufferSize_bytesIn)
{
	cxa_assert(usartIn);

	size_t numBytesRemaining = bufferSize_bytesIn;
	size_t numBytesSent = 0;
	while( numBytesRemaining != 0 )
	{
		ssize_t retVal_write = write(usartIn->fd, (void*)&(((uint8_t*)buffIn)[numBytesSent]), numBytesRemaining);
		if( retVal_write < 0 ) return false;

		// if we made it here, retVal_write is positive
		numBytesSent += (size_t)retVal_write;
		numBytesRemaining -= (size_t)retVal_write;
	}

	return true;
}


/*
FILE* cxa_usart_getFileDescriptor(cxa_usart_t *const superIn)
{
	cxa_assert(superIn);
	cxa_x86posix_usart_t *const usartIn = (cxa_x86posix_usart_t *const)superIn;

	return usartIn->fd;
}
*/


// ******** local function implementations ********
static bool set_interface_attribs (int fd, int speed, int parity)
{
	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0) return -1;

	cfsetospeed (&tty, speed);
	cfsetispeed (&tty, speed);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
	// disable IGNBRK for mismatched speed tests; otherwise receive break
	// as \000 chars
	tty.c_iflag &= ~IGNBRK;         // disable break processing
	tty.c_lflag = 0;                // no signaling chars, no echo,
	// no canonical processing
	tty.c_oflag = 0;                // no remapping, no delays
	tty.c_cc[VMIN]  = 0;            // read doesn't block
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

	tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
	// enable reading
	tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	if (tcsetattr (fd, TCSANOW, &tty) != 0) false;
	return true;
}


static bool set_blocking (int fd, int should_block)
{
	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0) return false;

	tty.c_cc[VMIN]  = should_block ? 1 : 0;
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

	if (tcsetattr (fd, TCSANOW, &tty) != 0) return false;

	return true;
}
