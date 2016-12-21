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
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>


#include <cxa_assert.h>
#include <cxa_config.h>
#include <cxa_numberUtils.h>
#include <cxa_timeDiff.h>


// ******** local macro definitions ********
#ifndef CXA_IOSTREAM_FORMATTED_BUFFERLEN_BYTES
	#define CXA_IOSTREAM_FORMATTED_BUFFERLEN_BYTES				24
#endif

#ifndef CXA_IOSTREAM_MAX_NUM_CLEARED_BYTES
	#define CXA_IOSTREAM_MAX_NUM_CLEARED_BYTES					80
#endif


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


bool cxa_ioStream_waitForCharSequence_withTimeout(cxa_ioStream_t *const ioStreamIn, const char* targetSeqIn, uint32_t timeout_msIn)
{
	cxa_assert(ioStreamIn);
	cxa_assert(targetSeqIn);

	cxa_timeDiff_t td_timeout;
	cxa_timeDiff_init(&td_timeout);

	uint8_t rxByte;
	while( !cxa_timeDiff_isElapsed_ms(&td_timeout, timeout_msIn) )
	{
		// see if we've found the end of the stream
		if( *targetSeqIn == 0 ) return true;

		// haven't found the end of the stream...keep reading
		cxa_ioStream_readStatus_t readStat = cxa_ioStream_readByte(ioStreamIn, &rxByte);
		if( readStat == CXA_IOSTREAM_READSTAT_ERROR ) return false;

		// no error, see if we got data
		if( readStat == CXA_IOSTREAM_READSTAT_GOTDATA )
		{
			// got data, see if it matches
			if( rxByte != *targetSeqIn ) return false;

			// it matches, move on to the next byte
			targetSeqIn++;
		}
	}

	return false;
}


void cxa_ioStream_clearReadBuffer(cxa_ioStream_t *const ioStreamIn)
{
	cxa_assert(ioStreamIn);

	for( int i = 0; i < CXA_IOSTREAM_MAX_NUM_CLEARED_BYTES; i++ )
	{
		if( cxa_ioStream_readByte(ioStreamIn, NULL) != CXA_IOSTREAM_READSTAT_GOTDATA ) return;
	}
}


bool cxa_ioStream_writeByte(cxa_ioStream_t *const ioStreamIn, uint8_t byteIn)
{
	cxa_assert(ioStreamIn);

	return cxa_ioStream_writeBytes(ioStreamIn, &byteIn, sizeof(byteIn));
}


bool cxa_ioStream_writeBytes(cxa_ioStream_t *const ioStreamIn, void* buffIn, size_t bufferSize_bytesIn)
{
	cxa_assert(ioStreamIn);
	if( bufferSize_bytesIn > 0 ) cxa_assert(buffIn);

	// make sure we're bound
	if( !cxa_ioStream_isBound(ioStreamIn) ) return false;

	return ioStreamIn->writeCb(buffIn, bufferSize_bytesIn, ioStreamIn->userVar);
}


bool cxa_ioStream_writeFixedByteBuffer(cxa_ioStream_t *const ioStreamIn, cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(ioStreamIn);
	cxa_assert(fbbIn);

	size_t size_bytes = cxa_fixedByteBuffer_getSize_bytes(fbbIn);

	return (size_bytes == 0) ? true : cxa_ioStream_writeBytes(ioStreamIn, (void*)cxa_fixedByteBuffer_get_pointerToIndex(fbbIn, 0), size_bytes);
}


bool cxa_ioStream_writeString(cxa_ioStream_t *const ioStreamIn, const char* stringIn)
{
	cxa_assert(ioStreamIn);
	cxa_assert(stringIn);

	return cxa_ioStream_writeBytes(ioStreamIn, (void*)stringIn, strlen(stringIn));
}


bool cxa_ioStream_writeLine(cxa_ioStream_t *const ioStreamIn, const char* stringIn)
{
	cxa_assert(ioStreamIn);
	cxa_assert(stringIn);

	if( !cxa_ioStream_writeString(ioStreamIn, stringIn) ) return false;
	return cxa_ioStream_writeString(ioStreamIn, CXA_LINE_ENDING);
}


bool cxa_ioStream_writeFormattedString(cxa_ioStream_t *const ioStreamIn, const char* formatIn, ...)
{
	cxa_assert(ioStreamIn);
	cxa_assert(formatIn);

	bool retVal = false;

	// now do our VARARGS
	va_list varArgs;
	va_start(varArgs, formatIn);
	retVal = cxa_ioStream_vWriteString(ioStreamIn, formatIn, varArgs, false, NULL);
	va_end(varArgs);

	return retVal;
}


bool cxa_ioStream_writeFormattedLine(cxa_ioStream_t *const ioStreamIn, const char* formatIn, ...)
{
	cxa_assert(ioStreamIn);
	cxa_assert(formatIn);

	bool retVal = false;

	// now do our VARARGS
	va_list varArgs;
	va_start(varArgs, formatIn);
	retVal = cxa_ioStream_vWriteString(ioStreamIn, formatIn, varArgs, false, NULL);
	va_end(varArgs);

	// add the EOL
	if( retVal ) retVal = cxa_ioStream_writeString(ioStreamIn, CXA_LINE_ENDING);

	return retVal;
}


bool cxa_ioStream_vWriteString(cxa_ioStream_t *const ioStreamIn,
								const char* formatIn, va_list argsIn,
								bool truncateIfTooLargeIn, const char* truncateStringIn)
{
	cxa_assert(ioStreamIn);
	cxa_assert(formatIn);

	bool retVal = false;

	// our buffer for this go-round
	char buff[CXA_IOSTREAM_FORMATTED_BUFFERLEN_BYTES];

	// now do our VARARGS
	size_t expectedNumBytesWritten = vsnprintf(buff, sizeof(buff), formatIn, argsIn);
	if( sizeof(buff) >= expectedNumBytesWritten )
	{
		// our buffer fits the string...write it
		retVal = cxa_ioStream_writeBytes(ioStreamIn, buff, CXA_MIN(expectedNumBytesWritten, sizeof(buff)));
	}
	else if( truncateIfTooLargeIn)
	{
		// our buffer doesn't fit the string, but we've been instructed to truncate
		retVal = cxa_ioStream_writeBytes(ioStreamIn, buff, CXA_MIN(expectedNumBytesWritten, sizeof(buff)));
		if( truncateStringIn != NULL ) retVal = cxa_ioStream_writeString(ioStreamIn, (char*)truncateStringIn);
	}

	return retVal;
}


// ******** local function implementations ********

