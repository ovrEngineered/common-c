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
#include "cxa_ioStream.h"


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


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_ioStream_init(cxa_ioStream_t *const ioStreamIn)
{
	cxa_assert(ioStreamIn);

	// setup our internal state
	cxa_ioStream_unbind(ioStreamIn);
}


void cxa_ioStream_bind(cxa_ioStream_t *const ioStreamIn, cxa_ioStream_cb_readByte_t readCbIn, cxa_ioStream_cb_writeBytes_t writeCbIn, void *const userVarIn)
{
	cxa_assert(ioStreamIn);

	// save our references
	ioStreamIn->readCb = readCbIn;
	ioStreamIn->writeCb = writeCbIn;
	ioStreamIn->userVar = userVarIn;
}


void cxa_ioStream_unbind(cxa_ioStream_t *const ioStreamIn)
{
	cxa_assert(ioStreamIn);

	ioStreamIn->readCb = NULL;
	ioStreamIn->writeCb = NULL;
	ioStreamIn->userVar = NULL;
}


bool cxa_ioStream_isBound(cxa_ioStream_t *const ioStreamIn)
{
	cxa_assert(ioStreamIn);

	return ((ioStreamIn->readCb != NULL) && (ioStreamIn->writeCb != NULL));
}


cxa_ioStream_readStatus_t cxa_ioStream_readByte(cxa_ioStream_t *const ioStreamIn, uint8_t *const byteOut)
{
	cxa_assert(ioStreamIn);

	// make sure we're bound
	if( !cxa_ioStream_isBound(ioStreamIn) ) return CXA_IOSTREAM_READSTAT_ERROR;

	return ioStreamIn->readCb(byteOut, ioStreamIn->userVar);
}


bool cxa_ioStream_writeByte(cxa_ioStream_t *const ioStreamIn, uint8_t byteIn)
{
	cxa_assert(ioStreamIn);

	return cxa_ioStream_writeBytes(ioStreamIn, &byteIn, sizeof(byteIn));
}


bool cxa_ioStream_writeBytes(cxa_ioStream_t *const ioStreamIn, void* buffIn, size_t bufferSize_bytesIn)
{
	cxa_assert(ioStreamIn);

	// make sure we're bound
	if( !cxa_ioStream_isBound(ioStreamIn) ) return false;

	return ioStreamIn->writeCb(buffIn, bufferSize_bytesIn, ioStreamIn->userVar);
}


bool cxa_ioStream_writeFixedByteBuffer(cxa_ioStream_t *const ioStreamIn, cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(ioStreamIn);
	cxa_assert(fbbIn);

	return cxa_ioStream_writeBytes(ioStreamIn, (void*)cxa_fixedByteBuffer_get_pointerToIndex(fbbIn, 0), cxa_fixedByteBuffer_getSize_bytes(fbbIn));
}


// ******** local function implementations ********

