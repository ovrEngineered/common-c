/**
 * Copyright 2013-2015 opencxa.org
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
#include "cxa_ioStream_fromFile.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <cxa_assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static bool set_blocking(cxa_ioStream_fromFile_t *const ioStreamIn, bool should_block);

static cxa_ioStream_readStatus_t read_cb(uint8_t *const byteOut, void *const userVarIn);
static bool write_cb(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_ioStream_fromFile_init(cxa_ioStream_fromFile_t *const ioStreamIn, FILE *const fileIn)
{
	cxa_assert(ioStreamIn);
	cxa_assert(fileIn);

	// save our internal state
	ioStreamIn->file = fileIn;

	// initialize our super class
	cxa_ioStream_init(&ioStreamIn->super);
	cxa_ioStream_bind(&ioStreamIn->super, read_cb, write_cb, (void*)ioStreamIn);

	// make our file non-blocking
	set_blocking(ioStreamIn, true);
}


void cxa_ioStream_fromFile_close(cxa_ioStream_fromFile_t *const ioStreamIn)
{
	cxa_assert(ioStreamIn);

	set_blocking(ioStreamIn, false);
}


// ******** local function implementations ********
static bool set_blocking(cxa_ioStream_fromFile_t *const ioStreamIn, bool should_block)
{
	cxa_assert(ioStreamIn);

	int fd = fileno(ioStreamIn->file);
	cxa_assert(fd >= 0);

	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0) return false;

	tty.c_cc[VMIN]  = should_block ? 1 : 0;
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

	if (tcsetattr (fd, TCSANOW, &tty) != 0) return false;

	return true;
}


static cxa_ioStream_readStatus_t read_cb(uint8_t *const byteOut, void *const userVarIn)
{
	cxa_assert(userVarIn);
	cxa_ioStream_fromFile_t* ioStreamIn = (cxa_ioStream_fromFile_t*)userVarIn;

	int fd = fileno(ioStreamIn->file);
	cxa_assert(fd >= 0);

	// perform our read and check the return value
	ssize_t retVal_read = read(fd, byteOut, 1);
	if( retVal_read < 0 ) return CXA_IOSTREAM_READSTAT_ERROR;
	else if( retVal_read == 0 ) return CXA_IOSTREAM_READSTAT_NODATA;

	return CXA_IOSTREAM_READSTAT_GOTDATA;
}


static bool write_cb(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn)
{
	cxa_assert(userVarIn);
	cxa_ioStream_fromFile_t* ioStreamIn = (cxa_ioStream_fromFile_t*)userVarIn;
	if( buffIn == NULL ) return false;

	for( size_t i = 0; i < bufferSize_bytesIn; i++ )
	{
		fputc( (((char*)buffIn)[i]), ioStreamIn->file );
	}

	return true;
}
