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
static cxa_ioStream_readStatus_t read_cb_ep1(uint8_t *const byteOut, void *const userVarIn);
static bool write_cb_ep1(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn);
static cxa_ioStream_readStatus_t read_cb_ep2(uint8_t *const byteOut, void *const userVarIn);
static bool write_cb_ep2(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_ioStream_loopback_init(cxa_ioStream_loopback_t *const ioStreamIn)
{
	cxa_assert(ioStreamIn);

	// initialize our ioStreams
	cxa_ioStream_init(&ioStreamIn->endPoint1);
	cxa_ioStream_bind(&ioStreamIn->endPoint1, read_cb_ep1, write_cb_ep1, (void*)ioStreamIn);
	cxa_fixedFifo_initStd(&ioStreamIn->fifo_ep1Read, CXA_FF_ON_FULL_DROP, ioStreamIn->fifo_ep1Read_raw);

	cxa_ioStream_init(&ioStreamIn->endPoint2);
	cxa_ioStream_bind(&ioStreamIn->endPoint2, read_cb_ep2, write_cb_ep2, (void*)ioStreamIn);
	cxa_fixedFifo_initStd(&ioStreamIn->fifo_ep2Read, CXA_FF_ON_FULL_DROP, ioStreamIn->fifo_ep2Read_raw);
}

cxa_ioStream_t* cxa_ioStream_loopback_getEndpoint1(cxa_ioStream_loopback_t* const ioStreamIn)
{
	cxa_assert(ioStreamIn);
	return &ioStreamIn->endPoint1;
}


cxa_ioStream_t* cxa_ioStream_loopback_getEndpoint2(cxa_ioStream_loopback_t* const ioStreamIn)
{
	cxa_assert(ioStreamIn);
	return &ioStreamIn->endPoint2;
}


// ******** local function implementations ********
static cxa_ioStream_readStatus_t read_cb_ep1(uint8_t *const byteOut, void *const userVarIn)
{
	cxa_assert(userVarIn);
	cxa_ioStream_loopback_t* ioStreamIn = (cxa_ioStream_loopback_t*)userVarIn;

	return cxa_fixedFifo_dequeue(&ioStreamIn->fifo_ep1Read, byteOut) ? CXA_IOSTREAM_READSTAT_GOTDATA : CXA_IOSTREAM_READSTAT_NODATA;
}


static bool write_cb_ep1(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn)
{
	cxa_assert(userVarIn);
	cxa_ioStream_loopback_t* ioStreamIn = (cxa_ioStream_loopback_t*)userVarIn;
	if( buffIn == NULL ) return false;

	for( size_t i = 0; i < bufferSize_bytesIn; i++ )
	{
		if( !cxa_fixedFifo_queue(&ioStreamIn->fifo_ep2Read, (void*)&(((uint8_t*)buffIn)[i])) ) return false;
	}

	return true;
}


static cxa_ioStream_readStatus_t read_cb_ep2(uint8_t *const byteOut, void *const userVarIn)
{
	cxa_assert(userVarIn);
	cxa_ioStream_loopback_t* ioStreamIn = (cxa_ioStream_loopback_t*)userVarIn;

	return cxa_fixedFifo_dequeue(&ioStreamIn->fifo_ep2Read, byteOut) ? CXA_IOSTREAM_READSTAT_GOTDATA : CXA_IOSTREAM_READSTAT_NODATA;
}


static bool write_cb_ep2(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn)
{
	cxa_assert(userVarIn);
	cxa_ioStream_loopback_t* ioStreamIn = (cxa_ioStream_loopback_t*)userVarIn;
	if( buffIn == NULL ) return false;

	for( size_t i = 0; i < bufferSize_bytesIn; i++ )
	{
		if( !cxa_fixedFifo_queue(&ioStreamIn->fifo_ep1Read, (void*)&(((uint8_t*)buffIn)[i])) ) return false;
	}

	return true;
}
