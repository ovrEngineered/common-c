/**
 * Copyright 2015 opencxa.org
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
#include "cxa_xmega_ioStream_toFile.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_array.h>
#include <cxa_fixedFifo.h>

#include <cxa_config.h>


// ******** local macro definitions ********
#ifndef CXA_XMEGA_IOSTREAM_MAX_FILES
	#define CXA_XMEGA_IOSTREAM_MAX_FILES 1
#endif


// ******** local type definitions ********


// ******** local function prototypes ********
static void init(void);
static int fdev_cb_putChar(char charIn, FILE *fdIn);
static int fdev_cb_getChar(FILE *fdIn);


// ********  local variable declarations *********
static cxa_array_t files;
static FILE files_raw[CXA_XMEGA_IOSTREAM_MAX_FILES];
static bool isInit = false;


// ******** global function implementations ********
FILE* cxa_xmega_ioStream_toFile(cxa_ioStream_t *const ioStreamIn)
{
	if( !isInit ) init();

	// create our new file
	FILE* newFile = cxa_array_append_empty(&files);
	if( newFile == NULL ) return NULL;

	// initialize it
	fdev_setup_stream(newFile, fdev_cb_putChar, fdev_cb_getChar, _FDEV_SETUP_RW);
	fdev_set_udata(newFile, (void*)ioStreamIn);

	return newFile;
}


// ******** local function implementations ********
static void init(void)
{
	cxa_array_initStd(&files, files_raw);
	isInit = true;
}


static int fdev_cb_putChar(char charIn, FILE *fdIn)
{
	cxa_assert(fdIn);

	// get a reference to our ioStream
	cxa_ioStream_t *const ioStream = (cxa_ioStream_t *const)fdev_get_udata(fdIn);
	cxa_assert(ioStream);

	cxa_ioStream_writeByte(ioStream, (char)charIn);

	// return 0 on success
	return 0;
}


static int fdev_cb_getChar(FILE *fdIn)
{
	cxa_assert(fdIn);

	// get a reference to our ioStream
	cxa_ioStream_t *const ioStream = (cxa_ioStream_t *const)fdev_get_udata(fdIn);
	cxa_assert(ioStream);

	uint8_t rxChar;
	int retVal = _FDEV_ERR;
	switch( cxa_ioStream_readByte(ioStream, &rxChar) )
	{
		case CXA_IOSTREAM_READSTAT_GOTDATA:
			retVal = (int)rxChar;
			break;

		case CXA_IOSTREAM_READSTAT_NODATA:
			retVal = _FDEV_EOF;
			break;
			
		default: break;
	}

	return retVal;
}
