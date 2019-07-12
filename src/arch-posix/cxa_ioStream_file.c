/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_ioStream_file.h"


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
static bool set_blocking(cxa_ioStream_file_t *const ioStreamIn, bool should_block);

static cxa_ioStream_readStatus_t read_cb(uint8_t *const byteOut, void *const userVarIn);
static bool write_cb(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_ioStream_file_init(cxa_ioStream_file_t *const ioStreamIn)
{
	cxa_assert(ioStreamIn);

	// initialize our super class
	cxa_ioStream_init(&ioStreamIn->super);
}


void cxa_ioStream_file_setFile(cxa_ioStream_file_t *const ioStreamIn, FILE *const fileIn)
{
	cxa_assert(ioStreamIn);
	cxa_assert(fileIn);

	// make sure we clean up previously used files
	if( cxa_ioStream_isBound(&ioStreamIn->super) )
	{
		cxa_ioStream_file_close(ioStreamIn);
	}

	// save our references
	ioStreamIn->file = fileIn;

	// make our file non-blocking
	set_blocking(ioStreamIn, true);

	// get ready for use
	cxa_ioStream_bind(&ioStreamIn->super, read_cb, write_cb, (void*)ioStreamIn);
}


void cxa_ioStream_file_close(cxa_ioStream_file_t *const ioStreamIn)
{
	cxa_assert(ioStreamIn);

	cxa_ioStream_unbind(&ioStreamIn->super);

	if( ioStreamIn->file != NULL )
	{
		set_blocking(ioStreamIn, false);
		fclose(ioStreamIn->file);
	}
}


// ******** local function implementations ********
static bool set_blocking(cxa_ioStream_file_t *const ioStreamIn, bool should_block)
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
	cxa_ioStream_file_t* ioStreamIn = (cxa_ioStream_file_t*)userVarIn;

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
	cxa_ioStream_file_t* ioStreamIn = (cxa_ioStream_file_t*)userVarIn;
	if( buffIn == NULL ) return false;

	for( size_t i = 0; i < bufferSize_bytesIn; i++ )
	{
		fputc( (((char*)buffIn)[i]), ioStreamIn->file );
	}

	return true;
}
