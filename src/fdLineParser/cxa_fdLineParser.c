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
#include "cxa_fdLineParser.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <cxa_assert.h>


// ******** local macro definitions ********
#define MAX_READ_BYTES_PER_UPDATE			16


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_fdLineParser_init(cxa_fdLineParser_t *const fdlpIn, FILE *fdIn, bool echoUserIn, void *bufferIn, size_t bufferSize_bytesIn, cxa_fdLineParser_lineCb_t cbIn, void *userVarIn)
{
	cxa_assert(fdlpIn);
	cxa_assert(fdIn);
	cxa_assert(bufferIn);
	cxa_assert(bufferSize_bytesIn >= 2);
	
	// save our references
	fdlpIn->fd = fdIn;
	fdlpIn->echoUser = echoUserIn;
	fdlpIn->cb = cbIn;
	fdlpIn->userVar = userVarIn;
	
	// setup our internal state
	cxa_array_init(&fdlpIn->lineBuffer, 1, bufferIn, bufferSize_bytesIn);
	fdlpIn->wasLastByteCr = false;
}


bool cxa_fdLineParser_update(cxa_fdLineParser_t *const fdlpIn)
{
	cxa_assert(fdlpIn);
	
	bool retVal = true;
	
	// limit how long we'll be in this function
	for( uint8_t i = 0; i < MAX_READ_BYTES_PER_UPDATE; i++ )
	{
		// get a byte from the file descriptor
		int newChar = fgetc(fdlpIn->fd);
		if( newChar == EOF ) break;
		
		size_t bufferSize_bytes = cxa_array_getSize_elems(&fdlpIn->lineBuffer);
		
		// we've got a valid byte...see if it's a line delimiter
		if( (newChar == '\r') || ((newChar == '\n') && !fdlpIn->wasLastByteCr) )
		{
			// we have a delimiter...terminate our string...this should always be
			// successful since we are carefully watching our size during appends
			uint8_t nullChar = 0;
			cxa_assert(cxa_array_append(&fdlpIn->lineBuffer, &nullChar));
			
			// call our callback
			if( fdlpIn->cb != NULL ) fdlpIn->cb((uint8_t*)cxa_array_get(&fdlpIn->lineBuffer, 0), bufferSize_bytes, fdlpIn->userVar);
			
			// clear our buffer
			cxa_array_clear(&fdlpIn->lineBuffer);
			fdlpIn->wasLastByteCr = (newChar == '\r');
			
			// exit the loop since the callback may have taken some time...
			break;
		}
		else if( newChar != '\n' )
		{
			// not a carriage return or line feed...make sure we have room in our buffer
			// for both this element AND a NULL termination upon delimiter
			if( (cxa_array_getFreeSize_elems(&fdlpIn->lineBuffer) < 2) || !cxa_array_append(&fdlpIn->lineBuffer, &newChar) )
			{
				// received too many characters before a CR, LF, or CRLF
				// clear our buffer and restart
				cxa_array_clear(&fdlpIn->lineBuffer);
				fdlpIn->wasLastByteCr = false;
				retVal = false;
			}
			else if( fdlpIn->echoUser ) fputc(newChar, fdlpIn->fd);
		}
	}
	
	return retVal;
}


// ******** local function implementations ********

