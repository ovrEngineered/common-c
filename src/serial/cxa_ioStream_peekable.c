/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_ioStream_peekable.h"


// ******** includes ********
#include <stdbool.h>

#include <cxa_assert.h>
#include <cxa_runLoop.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static cxa_ioStream_readStatus_t read_cb(uint8_t *const byteOut, void *const userVarIn);
static bool write_cb(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_ioStream_peekable_init(cxa_ioStream_peekable_t *const ioStreamIn,
								cxa_ioStream_t *const underlyingStreamIn)
{
	cxa_assert(ioStreamIn);
	cxa_assert(underlyingStreamIn);

	// save our references and set our initial state
	ioStreamIn->underlyingStream = underlyingStreamIn;
	ioStreamIn->hasBufferedByte = false;

	// initialize our super class
	cxa_ioStream_init(&ioStreamIn->super);
	cxa_ioStream_bind(&ioStreamIn->super, read_cb, write_cb, (void*)ioStreamIn);
}


cxa_ioStream_readStatus_t cxa_ioStream_peekable_hasBytesAvailable(cxa_ioStream_peekable_t *const ioStreamIn)
{
	cxa_assert(ioStreamIn);

	// simple case
	if( ioStreamIn->hasBufferedByte ) return CXA_IOSTREAM_READSTAT_GOTDATA;

	// if we made it here, we need to interact with the underlying stream...
	uint8_t rxByte;
	cxa_ioStream_readStatus_t readStat = cxa_ioStream_readByte(ioStreamIn->underlyingStream, &rxByte);

	if( readStat == CXA_IOSTREAM_READSTAT_GOTDATA )
	{
		// add it to our buffer so we don't lose it
		ioStreamIn->bufferedByte = rxByte;
		ioStreamIn->hasBufferedByte = true;
	}

	return readStat;
}


// ******** local function implementations ********
static cxa_ioStream_readStatus_t read_cb(uint8_t *const byteOut, void *const userVarIn)
{
	cxa_ioStream_peekable_t *const ioStreamIn = (cxa_ioStream_peekable_t*)userVarIn;
	cxa_assert(ioStreamIn);

	cxa_ioStream_readStatus_t retVal = CXA_IOSTREAM_READSTAT_NODATA;

	if( ioStreamIn->hasBufferedByte )
	{
		// we have data in our fifo to read first...
		if( byteOut != NULL ) *byteOut = ioStreamIn->bufferedByte;
		ioStreamIn->hasBufferedByte = false;
		retVal = CXA_IOSTREAM_READSTAT_GOTDATA;
	}
	else
	{
		// we're out of data in our fifo, call through to the underlying stream...
		retVal = cxa_ioStream_readByte(ioStreamIn->underlyingStream, byteOut);
	}

	return retVal;
}


static bool write_cb(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn)
{
	cxa_ioStream_peekable_t *const ioStreamIn = (cxa_ioStream_peekable_t*)userVarIn;
	cxa_assert(ioStreamIn);

	// call through to the underlying stream...
	return cxa_ioStream_writeBytes(ioStreamIn->underlyingStream, buffIn, bufferSize_bytesIn);
}
