/**
 * @copyright 2016 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Christopher Armenio
 */
#include "cxa_ioStream_nullablePassthrough.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_runLoop.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static cxa_ioStream_readStatus_t cb_ioStream_readByte(uint8_t *const byteOut, void *const userVarIn);
static bool cb_ioStream_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_ioStream_nullablePassthrough_init(cxa_ioStream_nullablePassthrough_t *const npIn)
{
	cxa_assert(npIn);

	npIn->numBytesWritten = 0;

	// setup our nonnull ioStream
	cxa_ioStream_init(&npIn->nonnullStream);
	cxa_ioStream_bind(&npIn->nonnullStream, cb_ioStream_readByte, cb_ioStream_writeBytes, (void*)npIn);

	// and our nullable stream
	npIn->nullableStream = NULL;
}


cxa_ioStream_t* cxa_ioStream_nullablePassthrough_getNonullStream(cxa_ioStream_nullablePassthrough_t *const npIn)
{
	cxa_assert(npIn);

	return &npIn->nonnullStream;
}


cxa_ioStream_t* cxa_ioStream_nullablePassthrough_getNullableStream(cxa_ioStream_nullablePassthrough_t *const npIn)
{
	cxa_assert(npIn);

	return npIn->nullableStream;
}


void cxa_ioStream_nullablePassthrough_setNullableStream(cxa_ioStream_nullablePassthrough_t *const npIn,
														cxa_ioStream_t *const ioStreamIn)
{
	cxa_assert(npIn);

	// save our new ioStream
	npIn->nullableStream = ioStreamIn;
}


size_t cxa_ioStream_nullablePassthrough_getNumBytesWritten(cxa_ioStream_nullablePassthrough_t *const npIn)
{
	cxa_assert(npIn);

	return npIn->numBytesWritten;
}


void cxa_ioStream_nullablePassthrough_resetNumByesWritten(cxa_ioStream_nullablePassthrough_t *const npIn)
{
	cxa_assert(npIn);

	npIn->numBytesWritten = 0;
}


// ******** local function implementations ********
static cxa_ioStream_readStatus_t cb_ioStream_readByte(uint8_t *const byteOut, void *const userVarIn)
{
	cxa_ioStream_nullablePassthrough_t *const npIn = (cxa_ioStream_nullablePassthrough_t*)userVarIn;
	cxa_assert(npIn);

	if( npIn->nullableStream == NULL ) return CXA_IOSTREAM_READSTAT_NODATA;

	return cxa_ioStream_readByte(npIn->nullableStream, byteOut);
}


static bool cb_ioStream_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn)
{
	cxa_ioStream_nullablePassthrough_t *const npIn = (cxa_ioStream_nullablePassthrough_t*)userVarIn;
	cxa_assert(npIn);

	npIn->numBytesWritten += bufferSize_bytesIn;

	if( npIn->nullableStream == NULL ) return true;

	return cxa_ioStream_writeBytes(npIn->nullableStream, buffIn, bufferSize_bytesIn);
}

