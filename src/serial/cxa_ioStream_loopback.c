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
#include "cxa_ioStream_loopback.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static cxa_ioStream_readStatus_t read_cb(uint8_t *const byteOut, void *const userVarIn);
static bool write_cb(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_ioStream_loopback_init(cxa_ioStream_loopback_t *const ioStreamIn)
{
	cxa_assert(ioStreamIn);

	// initialize our fifo
	cxa_fixedFifo_initStd(&ioStreamIn->fifo, CXA_FF_ON_FULL_DROP, ioStreamIn->fifo_raw);

	// initialize our super class
	cxa_ioStream_init(&ioStreamIn->super);
	cxa_ioStream_bind(&ioStreamIn->super, read_cb, write_cb, (void*)ioStreamIn);
}


// ******** local function implementations ********
static cxa_ioStream_readStatus_t read_cb(uint8_t *const byteOut, void *const userVarIn)
{
	cxa_assert(userVarIn);
	cxa_ioStream_loopback_t* ioStreamIn = (cxa_ioStream_loopback_t*)userVarIn;

	return cxa_fixedFifo_dequeue(&ioStreamIn->fifo, byteOut) ? CXA_IOSTREAM_READSTAT_GOTDATA : CXA_IOSTREAM_READSTAT_NODATA;
}


static bool write_cb(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn)
{
	cxa_assert(userVarIn);
	cxa_ioStream_loopback_t* ioStreamIn = (cxa_ioStream_loopback_t*)userVarIn;
	if( buffIn == NULL ) return false;

	for( size_t i = 0; i < bufferSize_bytesIn; i++ )
	{
		if( !cxa_fixedFifo_queue(&ioStreamIn->fifo, (void*)&(((uint8_t*)buffIn)[i])) ) return false;
	}

	return true;
}
