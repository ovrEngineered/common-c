/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_ioStream_loopback.h"


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
