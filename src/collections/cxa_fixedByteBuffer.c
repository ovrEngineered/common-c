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
#include "cxa_fixedByteBuffer.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <cxa_assert.h>
#include <string.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_fixedByteBuffer_init(cxa_fixedByteBuffer_t *const fbbIn, void *const bufferLocIn, const size_t bufferMaxSize_bytesIn)
{
	cxa_assert(fbbIn);
	cxa_assert(bufferLocIn);
	cxa_assert(bufferMaxSize_bytesIn > 0);
	
	// setup our internal state
	cxa_array_init(&fbbIn->bytes, 1, bufferLocIn, bufferMaxSize_bytesIn);
}


bool cxa_fixedByteBuffer_append(cxa_fixedByteBuffer_t *const fbbIn, uint8_t *const ptrIn, const size_t numBytesIn)
{
	cxa_assert(fbbIn);
	cxa_assert(ptrIn);

	// make sure we have room for the operation
	if( cxa_fixedByteBuffer_getFreeSize_bytes(fbbIn) < numBytesIn ) return false;

	for( size_t i = 0; i < numBytesIn; i++ )
	{
		if( !cxa_array_append(&fbbIn->bytes, &(ptrIn[i])) ) return false;
	}

	return true;
}


bool cxa_fixedByteBuffer_remove(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, const size_t numBytesIn)
{
	cxa_assert(fbbIn);

	// make sure we have room for the operation
	if( (indexIn + numBytesIn) > cxa_fixedByteBuffer_getSize_bytes(fbbIn) ) return false;

	for( size_t i = 0; i < numBytesIn; i++ )
	{
		if( !cxa_array_remove_atIndex(&fbbIn->bytes, indexIn) ) return false;
	}

	return true;
}


bool cxa_fixedByteBuffer_remove_cString(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn)
{
	cxa_assert(fbbIn);

	uint8_t* targetString = cxa_fixedByteBuffer_get_pointerToIndex(fbbIn, indexIn);
	if( targetString == NULL ) return false;

	// figure out how long the string is...
	size_t strLen_bytes = strlen((const char*)targetString) + 1;

	return cxa_fixedByteBuffer_remove(fbbIn, indexIn, strLen_bytes);
}


uint8_t* cxa_fixedByteBuffer_get_pointerToIndex(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn)
{
	cxa_assert(fbbIn);

	return (uint8_t*)cxa_array_get(&fbbIn->bytes, indexIn);
}


bool cxa_fixedByteBuffer_get(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, bool transposeIn, uint8_t *const valOut, const size_t numBytesIn)
{
	cxa_assert(fbbIn);

	// make sure we have enough bytes in the buffer for this operation
	if( (indexIn + numBytesIn) > cxa_fixedByteBuffer_getSize_bytes(fbbIn)) return false;

	// if we don't have anyplace to copy the data, we're done!
	if( !valOut ) return true;

	// we need to copy the data out
	for( size_t i = 0; i < numBytesIn; i++ )
	{
		uint8_t *currByte = cxa_array_get(&fbbIn->bytes, indexIn+i);
		if( currByte == NULL ) return false;

		if( !transposeIn ) valOut[i] = *currByte;
		else valOut[numBytesIn-i-1] = *currByte;
	}

	return true;
}


bool cxa_fixedByteBuffer_get_cString(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, char *const stringOut, size_t maxOutputSize_bytes)
{
	cxa_assert(fbbIn);
	cxa_assert(stringOut);

	// make sure we have enough bytes in the buffer for this operation
	char* targetString = (char*)cxa_array_get(&fbbIn->bytes, indexIn);
	if( targetString == NULL) return false;

	// size+1 for terminator
	size_t targetStringSize_bytes = strlen(targetString)+1;

	if( targetStringSize_bytes > maxOutputSize_bytes ) return false;

	// if we don't have anyplace to copy the data, we're done!
	if( !stringOut ) return true;

	memcpy(stringOut, targetString, targetStringSize_bytes);

	return true;
}


bool cxa_fixedByteBuffer_replace(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, uint8_t *const ptrIn, const size_t numBytesIn)
{
	cxa_assert(fbbIn);
	cxa_assert(ptrIn);

	// make sure we have enough bytes in the buffer for this operation
	if( (indexIn + numBytesIn) > cxa_fixedByteBuffer_getSize_bytes(fbbIn)) return false;

	for( int i = 0; i < numBytesIn; i++ )
	{
		if( !cxa_array_overwrite(&fbbIn->bytes, indexIn+i, &(ptrIn[i])) ) return false;
	}

	return true;
}



bool cxa_fixedByteBuffer_replace_cString(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, char *const stringIn)
{
	cxa_assert(fbbIn);
	cxa_assert(stringIn);

	// calculate our sizes
	size_t replacementStringSize_bytes = strlen(stringIn)+1;
	char* targetString = (char*)cxa_fixedByteBuffer_get_pointerToIndex(fbbIn, indexIn);
	if( targetString == NULL ) return false;

	size_t targetStringSize_bytes = strlen(targetString)+1;
	ssize_t discrepantSize_bytes = replacementStringSize_bytes - targetStringSize_bytes;

	// make sure we have room for this operation at the specified index
	if( (indexIn+replacementStringSize_bytes) > cxa_fixedByteBuffer_getMaxSize_bytes(fbbIn) ) return false;

	// now, make sure that we have enough free space in the buffer if the replacement string is larger
	if( (discrepantSize_bytes > 0) && (cxa_fixedByteBuffer_getFreeSize_bytes(fbbIn) < discrepantSize_bytes) ) return false;

	// if we made it here, we should be good to perform the operation...start by removing the current string
	if( !cxa_fixedByteBuffer_remove_cString(fbbIn, indexIn) ) return false;

	// now insert the new string
	return cxa_fixedByteBuffer_insert_cString(fbbIn, indexIn, stringIn);
}


bool cxa_fixedByteBuffer_insert(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, uint8_t *const ptrIn, const size_t numBytesIn)
{
	cxa_assert(fbbIn);
	cxa_assert(ptrIn);

	// make sure we have room for the operation
	if( cxa_fixedByteBuffer_getFreeSize_bytes(fbbIn) < numBytesIn ) return false;

	// make sure the index is in bounds
	if( indexIn > cxa_fixedByteBuffer_getSize_bytes(fbbIn) ) return false;

	for( size_t i = 0; i < numBytesIn; i++ )
	{
		if( !cxa_array_insert(&fbbIn->bytes, indexIn+i, (void*)&(ptrIn[i])) ) return false;
	}

	return true;
}


size_t cxa_fixedByteBuffer_getSize_bytes(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);

	return cxa_array_getSize_elems(&fbbIn->bytes);
}


size_t cxa_fixedByteBuffer_getMaxSize_bytes(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);

	return cxa_array_getMaxSize_elems(&fbbIn->bytes);
}


size_t cxa_fixedByteBuffer_getFreeSize_bytes(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);

	return cxa_array_getFreeSize_elems(&fbbIn->bytes);
}


bool cxa_fixedByteBuffer_isFull(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);
	return cxa_fixedByteBuffer_getFreeSize_bytes(fbbIn) == 0;
}


bool cxa_fixedByteBuffer_isEmpty(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);
	return cxa_fixedByteBuffer_getSize_bytes(fbbIn) == 0;
}


void cxa_fixedByteBuffer_clear(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);

	cxa_array_clear(&fbbIn->bytes);
}


bool cxa_fixedByteBuffer_writeToFile_bytes(cxa_fixedByteBuffer_t *const fbbIn, FILE *fileIn)
{
	cxa_assert(fbbIn);
	cxa_assert(fileIn);
	
	for( size_t i = 0; i < cxa_fixedByteBuffer_getSize_bytes(fbbIn); i++ )
	{
		uint8_t currByte;
		if( !cxa_fixedByteBuffer_get_uint8(fbbIn, i, currByte) ) return false;

		if( fputc(currByte, fileIn) < 0 ) return false;
	}
	
	return true;
}


bool cxa_fixedByteBuffer_writeToFile_asciiHexRep(cxa_fixedByteBuffer_t *const fbbIn, FILE *fileIn)
{
	cxa_assert(fbbIn);
	cxa_assert(fileIn);
	
	if( fprintf(fileIn, "fixedByteBuffer @ %p { ", fbbIn) < 0 ) return false;
	for( size_t i = 0; i < cxa_fixedByteBuffer_getSize_bytes(fbbIn); i++ )
	{
		uint8_t currByte;
		if( !cxa_fixedByteBuffer_get_uint8(fbbIn, i, currByte) ) return false;

		if( fprintf(fileIn, "%02X", currByte) < 0 ) return false;
		
		if( i != (cxa_fixedByteBuffer_getSize_bytes(fbbIn)-1))
		{
			if( fputs(" ", fileIn) < 0 ) return false;
		}
	}
	if( fputs(" }\r\n", fileIn) < 0 ) return false;

	return true;
}


// ******** local function implementations ********

