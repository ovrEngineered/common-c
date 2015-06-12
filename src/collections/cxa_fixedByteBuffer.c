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
static void incrementSubBufferStartIndices(cxa_fixedByteBuffer_t *const fbbIn, bool isRootBufferIn, const size_t amountToIncrementIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_fixedByteBuffer_init(cxa_fixedByteBuffer_t *const fbbIn, void *const bufferLocIn, const size_t bufferMaxSize_bytesIn)
{
	cxa_assert(fbbIn);
	cxa_assert(bufferLocIn);
	cxa_assert(bufferMaxSize_bytesIn > 0);
	
	// setup our internal state
	cxa_array_init(&fbbIn->bytes, 1, bufferLocIn, bufferMaxSize_bytesIn);
	cxa_array_init(&fbbIn->subBuffers, sizeof(*fbbIn->subBuffers_raw), (void*)fbbIn->subBuffers_raw, sizeof(fbbIn->subBuffers_raw));
	
	// we have no parent
	fbbIn->parent = NULL;
}


bool cxa_fixedByteBuffer_init_subBuffer(cxa_fixedByteBuffer_t *const subFbbIn, cxa_fixedByteBuffer_t *const parentFbbIn,
		size_t startIndexIn, size_t maxSize_bytesIn)
{
	cxa_assert(subFbbIn);
	cxa_assert(parentFbbIn);

	// make sure our sizes / indices are appropriate
	if( (startIndexIn + maxSize_bytesIn) > cxa_fixedByteBuffer_getMaxSize(parentFbbIn) ) return false;

	// setup our internal state
	cxa_array_init(&subFbbIn->subBuffers, sizeof(*subFbbIn->subBuffers_raw), (void*)subFbbIn->subBuffers_raw, sizeof(subFbbIn->subBuffers_raw));

	// store our references
	subFbbIn->parent = parentFbbIn;
	cxa_array_append(&parentFbbIn->subBuffers, (void*)&subFbbIn);
	subFbbIn->subBuff_startIndexInParent = startIndexIn;
	subFbbIn->subBuff_maxSize_bytes = (maxSize_bytesIn == CXA_FIXED_BYTE_BUFFER_LEN_ALL) ? (cxa_fixedByteBuffer_getMaxSize(parentFbbIn) - startIndexIn) : maxSize_bytesIn;

	return true;
}


size_t cxa_fixedByteBuffer_getCurrSize(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);
	return (fbbIn->parent == NULL) ? cxa_array_getSize_elems(&fbbIn->bytes) : (cxa_fixedByteBuffer_getCurrSize(fbbIn->parent) - fbbIn->subBuff_startIndexInParent);
}


size_t cxa_fixedByteBuffer_getMaxSize(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);
	return (fbbIn->parent == NULL) ? cxa_array_getMaxSize_elems(&fbbIn->bytes) : fbbIn->subBuff_maxSize_bytes;
}


size_t cxa_fixedByteBuffer_getBytesRemaining(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);

	// depends on whether or not we have a parent
	if( fbbIn->parent == NULL )
	{
		return cxa_array_getFreeSize_elems(&fbbIn->bytes);
	}

	// if we made it here, little more complicated now since we have a parent...
	size_t retVal = cxa_fixedByteBuffer_getBytesRemaining(fbbIn->parent) - fbbIn->subBuff_startIndexInParent;
	return (retVal > fbbIn->subBuff_maxSize_bytes) ? fbbIn->subBuff_maxSize_bytes : retVal;
}


bool cxa_fixedByteBuffer_isEmpty(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);
	return cxa_fixedByteBuffer_getCurrSize(fbbIn) == 0;
}


bool cxa_fixedByteBuffer_isFull(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);
	return cxa_fixedByteBuffer_getBytesRemaining(fbbIn) == 0;
}


bool cxa_fixedByteBuffer_append_uint8(cxa_fixedByteBuffer_t *const fbbIn, uint8_t byteIn)
{
	cxa_assert(fbbIn);
	
	// depends on whether or not we have a parent
	if( fbbIn->parent == NULL )
	{
		// we do not have a parent...
		return cxa_array_append(&fbbIn->bytes, &byteIn);
	}

	// if we made it here, little more complicated now since we have a parent...
	return cxa_fixedByteBuffer_append_uint8(fbbIn->parent, byteIn);
}


bool cxa_fixedByteBuffer_append_uint16LE(cxa_fixedByteBuffer_t *const fbbIn, uint16_t uint16In)
{
	cxa_assert(fbbIn);
	
	// make sure we have room for the operation
	if( cxa_fixedByteBuffer_getBytesRemaining(fbbIn) < 2 ) return false;

	uint8_t tmp = (uint8_t)((uint16In & (uint16_t)0x00FF) >> 0);
	if( !cxa_fixedByteBuffer_append_uint8(fbbIn, tmp) ) return false;
	
	tmp = (uint8_t)((uint16In & (uint16_t)0xFF00) >> 8);
	if( !cxa_fixedByteBuffer_append_uint8(fbbIn, tmp) ) return false;
	
	return true;
}


bool cxa_fixedByteBuffer_append_uint32LE(cxa_fixedByteBuffer_t *const fbbIn, uint32_t uint32In)
{
	cxa_assert(fbbIn);

	// make sure we have room for the operation
	if( cxa_fixedByteBuffer_getBytesRemaining(fbbIn) < 4 ) return false;
		
	uint8_t tmp = (uint8_t)((uint32In & (uint32_t)0x000000FF) >> 0);
	if( !cxa_fixedByteBuffer_append_uint8(fbbIn, tmp) ) return false;
		
	tmp = (uint8_t)((uint32In & (uint32_t)0x0000FF00) >> 8);
	if( !cxa_fixedByteBuffer_append_uint8(fbbIn, tmp) ) return false;
	
	tmp = (uint8_t)((uint32In & (uint32_t)0x00FF0000) >> 16);
	if( !cxa_fixedByteBuffer_append_uint8(fbbIn, tmp) ) return false;
		
	tmp = (uint8_t)((uint32In & (uint32_t)0xFF000000) >> 24);
	if( !cxa_fixedByteBuffer_append_uint8(fbbIn, tmp) ) return false;
	
	return true;
}


bool cxa_fixedByteBuffer_append_floatLE(cxa_fixedByteBuffer_t *const fbbIn, float floatIn)
{
	cxa_assert(fbbIn);

	// make sure we have room for the operation
	if( cxa_fixedByteBuffer_getBytesRemaining(fbbIn) < 4 ) return false;

	uint8_t tmp = ((uint8_t*)&floatIn)[0];
	if( !cxa_fixedByteBuffer_append_uint8(fbbIn, tmp) ) return false;

	tmp = ((uint8_t*)&floatIn)[1];
	if( !cxa_fixedByteBuffer_append_uint8(fbbIn, tmp) ) return false;

	tmp = ((uint8_t*)&floatIn)[2];
	if( !cxa_fixedByteBuffer_append_uint8(fbbIn, tmp) ) return false;

	tmp = ((uint8_t*)&floatIn)[3];
	if( !cxa_fixedByteBuffer_append_uint8(fbbIn, tmp) ) return false;

	return true;
}


bool cxa_fixedByteBuffer_append_cString(cxa_fixedByteBuffer_t *const fbbIn, char *const stringIn)
{
	cxa_assert(fbbIn);
	cxa_assert(stringIn);

	size_t stringSize_bytes = strlen(stringIn);
	for( size_t i = 0; i < stringSize_bytes; i++ )
	{
		if( !cxa_fixedByteBuffer_append_uint8(fbbIn, (uint8_t)stringIn[i]) ) return false;
	}

	return true;
}


bool cxa_fixedByteBuffer_append_fbb(cxa_fixedByteBuffer_t *const fbbIn, cxa_fixedByteBuffer_t *const srcFbbIn)
{
	cxa_assert(fbbIn);
	cxa_assert(srcFbbIn);
	
	size_t numBytes_src = cxa_fixedByteBuffer_getCurrSize(srcFbbIn);
	if( numBytes_src > cxa_fixedByteBuffer_getBytesRemaining(fbbIn) ) return false;
	
	for( size_t i = 0; i < numBytes_src; i++ )
	{
		if( !cxa_fixedByteBuffer_append_uint8(fbbIn, cxa_fixedByteBuffer_get_uint8(srcFbbIn, i)) ) return false;
	}
	return true;
}


uint8_t* cxa_fixedByteBuffer_get_pointerToIndex(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn)
{
	cxa_assert(fbbIn);

	// depends on whether or not we have a parent
	if( fbbIn->parent == NULL )
	{
		// we do not have a parent...
		return (uint8_t*)cxa_array_getAtIndex(&fbbIn->bytes, indexIn);
	}

	// if we made it here, little more complicated now since we have a parent...
	return cxa_fixedByteBuffer_get_pointerToIndex(fbbIn->parent, indexIn + fbbIn->subBuff_startIndexInParent);
}


uint8_t cxa_fixedByteBuffer_get_uint8(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn)
{
	uint8_t* retVal = cxa_fixedByteBuffer_get_pointerToIndex(fbbIn, indexIn);
	cxa_assert(retVal);
	return *retVal;
}


uint16_t cxa_fixedByteBuffer_get_uint16LE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn)
{
	cxa_assert(fbbIn);
	
	uint8_t* byte0 = cxa_fixedByteBuffer_get_pointerToIndex(fbbIn, indexIn);
	uint8_t* byte1 = cxa_fixedByteBuffer_get_pointerToIndex(fbbIn, indexIn+1);
	
	cxa_assert(byte0);
	cxa_assert(byte1);

	return (((uint16_t)*byte0) <<  0) | (((uint16_t)*byte1) <<  8);
}


uint32_t cxa_fixedByteBuffer_get_uint32LE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn)
{
	cxa_assert(fbbIn);
	
	uint8_t* byte0 = cxa_fixedByteBuffer_get_pointerToIndex(fbbIn, indexIn);
	uint8_t* byte1 = cxa_fixedByteBuffer_get_pointerToIndex(fbbIn, indexIn+1);
	uint8_t* byte2 = cxa_fixedByteBuffer_get_pointerToIndex(fbbIn, indexIn+2);
	uint8_t* byte3 = cxa_fixedByteBuffer_get_pointerToIndex(fbbIn, indexIn+3);
	
	cxa_assert(byte0);
	cxa_assert(byte1);
	cxa_assert(byte2);
	cxa_assert(byte3);

	return (((uint32_t)*byte0) <<  0) | (((uint32_t)*byte1) <<  8) | (((uint32_t)*byte2) << 16) | (((uint32_t)*byte3) << 24);
}


float cxa_fixedByteBuffer_get_floatLE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn)
{
	cxa_assert(fbbIn);

	uint32_t intVal = cxa_fixedByteBuffer_get_uint32LE(fbbIn, indexIn);

	float retVal = 0;
	((uint8_t*)&retVal)[0] = ((uint8_t*)&intVal)[0];
	((uint8_t*)&retVal)[1] = ((uint8_t*)&intVal)[1];
	((uint8_t*)&retVal)[2] = ((uint8_t*)&intVal)[2];
	((uint8_t*)&retVal)[3] = ((uint8_t*)&intVal)[3];
	
	return retVal;
}


bool cxa_fixedByteBuffer_get_cString(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, char *const stringOut, size_t maxOutputSize_bytes)
{
	cxa_assert(fbbIn);
	cxa_assert(stringOut);

	for( size_t i = 0; i < maxOutputSize_bytes; i++ )
	{
		uint8_t* currByte = cxa_fixedByteBuffer_get_pointerToIndex(fbbIn, indexIn+i);
		if( currByte == NULL ) return false;

		stringOut[i] = *currByte;
		if( *currByte == 0 ) return true;
	}

	return false;
}


bool cxa_fixedByteBuffer_overwrite_uint8(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, uint8_t byteIn)
{
	cxa_assert(fbbIn);

	// depends on whether or not we have a parent
	if( fbbIn->parent == NULL )
	{
		// we do not have a parent...
		return cxa_array_overwriteAtIndex(&fbbIn->bytes, indexIn, &byteIn);
	}

	// if we made it here, little more complicated now since we have a parent...
	return cxa_fixedByteBuffer_overwrite_uint8(fbbIn->parent, (indexIn + fbbIn->subBuff_startIndexInParent), byteIn);
}


bool cxa_fixedByteBuffer_overwrite_uint16LE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, uint16_t uint16In)
{
	cxa_assert(fbbIn);

	// make sure we have room for this operation at the specified index
	if( (indexIn+2) > cxa_fixedByteBuffer_getCurrSize(fbbIn) ) return false;

	uint8_t tmp = (uint8_t)((uint16In & (uint16_t)0x00FF) >> 0);
	if( !cxa_fixedByteBuffer_overwrite_uint8(fbbIn, indexIn, tmp) ) return false;

	tmp = (uint8_t)((uint16In & (uint16_t)0xFF00) >> 8);
	if( !cxa_fixedByteBuffer_overwrite_uint8(fbbIn, indexIn+1, tmp) ) return false;

	return true;
}


bool cxa_fixedByteBuffer_overwrite_uint32LE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, uint16_t uint32In)
{
	cxa_assert(fbbIn);

	// make sure we have room for this operation at the specified index
	if( (indexIn+4) > cxa_fixedByteBuffer_getCurrSize(fbbIn) ) return false;

	uint8_t tmp = (uint8_t)((uint32In & (uint32_t)0x000000FF) >> 0);
	if( !cxa_fixedByteBuffer_overwrite_uint8(fbbIn, indexIn, tmp) ) return false;

	tmp = (uint8_t)((uint32In & (uint32_t)0x0000FF00) >> 8);
	if( !cxa_fixedByteBuffer_overwrite_uint8(fbbIn, indexIn+1, tmp) ) return false;

	tmp = (uint8_t)((uint32In & (uint32_t)0x00FF0000) >> 16);
	if( !cxa_fixedByteBuffer_overwrite_uint8(fbbIn, indexIn+2, tmp) ) return false;

	tmp = (uint8_t)((uint32In & (uint32_t)0xFF000000) >> 24);
	if( !cxa_fixedByteBuffer_overwrite_uint8(fbbIn, indexIn+3, tmp) ) return false;

	return true;
}


bool cxa_fixedByteBuffer_overwrite_floatLE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, float floatIn)
{
	cxa_assert(fbbIn);

	// make sure we have room for this operation at the specified index
	if( (indexIn+4) > cxa_fixedByteBuffer_getCurrSize(fbbIn) ) return false;

	uint8_t tmp = ((uint8_t*)&floatIn)[0];
	if( !cxa_fixedByteBuffer_overwrite_uint8(fbbIn, indexIn, tmp) ) return false;

	tmp = ((uint8_t*)&floatIn)[1];
	if( !cxa_fixedByteBuffer_overwrite_uint8(fbbIn, indexIn+1, tmp) ) return false;

	tmp = ((uint8_t*)&floatIn)[2];
	if( !cxa_fixedByteBuffer_overwrite_uint8(fbbIn, indexIn+2, tmp) ) return false;

	tmp = ((uint8_t*)&floatIn)[3];
	if( !cxa_fixedByteBuffer_overwrite_uint8(fbbIn, indexIn+3, tmp) ) return false;

	return true;
}


bool cxa_fixedByteBuffer_insert_uint8(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, uint8_t valueIn)
{
	cxa_assert(fbbIn);

	// depends on whether or not we have a parent
	if( fbbIn->parent == NULL )
	{
		// we do not have a parent...do our insert
		if( !cxa_array_insertAtIndex(&fbbIn->bytes, indexIn, &valueIn) ) return false;

		// increment the starting indices of our subBuffers
		incrementSubBufferStartIndices(fbbIn, true, 1);

		return true;
	}

	// if we made it here, little more complicated now since we have a parent...
	return cxa_fixedByteBuffer_insert_uint8(fbbIn->parent, (indexIn + fbbIn->subBuff_startIndexInParent), valueIn);
}


bool cxa_fixedByteBuffer_insert_uint16LE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, uint16_t valueIn)
{
	cxa_assert(fbbIn);

	// make sure we have room for the operation
	if( cxa_fixedByteBuffer_getBytesRemaining(fbbIn) < 2 ) return false;

	// depends on whether or not we have a parent
	if( fbbIn->parent == NULL )
	{
		// we do not have a parent...do our insert
		uint8_t tmp = (uint8_t)((valueIn & (uint16_t)0x00FF) >> 0);
		if( !cxa_fixedByteBuffer_insert_uint8(fbbIn, indexIn, tmp) ) return false;

		tmp = (uint8_t)((valueIn & (uint16_t)0xFF00) >> 8);
		if( !cxa_fixedByteBuffer_insert_uint8(fbbIn, indexIn+1, tmp) ) return false;

		// increment the starting indices of our subBuffers
		incrementSubBufferStartIndices(fbbIn, true, 2);

		return true;
	}

	// if we made it here, little more complicated now since we have a parent...
	return cxa_fixedByteBuffer_insert_uint16LE(fbbIn->parent, (indexIn + fbbIn->subBuff_startIndexInParent), valueIn);
}


bool cxa_fixedByteBuffer_insert_uint32LE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, uint32_t valueIn)
{
	cxa_assert(fbbIn);

	// make sure we have room for the operation
	if( cxa_fixedByteBuffer_getBytesRemaining(fbbIn) < 4 ) return false;

	// depends on whether or not we have a parent
	if( fbbIn->parent == NULL )
	{
		// we do not have a parent...do our insert
		uint8_t tmp = (uint8_t)((valueIn & (uint32_t)0x000000FF) >> 0);
		if( !cxa_fixedByteBuffer_insert_uint8(fbbIn, indexIn, tmp) ) return false;

		tmp = (uint8_t)((valueIn & (uint32_t)0x0000FF00) >> 8);
		if( !cxa_fixedByteBuffer_insert_uint8(fbbIn, indexIn+1, tmp) ) return false;

		tmp = (uint8_t)((valueIn & (uint32_t)0x00FF0000) >> 16);
		if( !cxa_fixedByteBuffer_insert_uint8(fbbIn, indexIn+2, tmp) ) return false;

		tmp = (uint8_t)((valueIn & (uint32_t)0xFF000000) >> 24);
		if( !cxa_fixedByteBuffer_insert_uint8(fbbIn, indexIn+3, tmp) ) return false;

		// increment the starting indices of our subBuffers
		incrementSubBufferStartIndices(fbbIn, true, 4);

		return true;
	}

	// if we made it here, little more complicated now since we have a parent...
	return cxa_fixedByteBuffer_insert_uint32LE(fbbIn->parent, (indexIn + fbbIn->subBuff_startIndexInParent), valueIn);
}


bool cxa_fixedByteBuffer_insert_floatLE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, float valueIn)
{
	cxa_assert(fbbIn);

	// make sure we have room for the operation
	if( cxa_fixedByteBuffer_getBytesRemaining(fbbIn) < 4 ) return false;

	// depends on whether or not we have a parent
	if( fbbIn->parent == NULL )
	{
		// we do not have a parent...do our insert
		uint8_t tmp = ((uint8_t*)&valueIn)[3];
		if( !cxa_fixedByteBuffer_insert_uint8(fbbIn, indexIn, tmp) ) return false;

		tmp = ((uint8_t*)&valueIn)[2];
		if( !cxa_fixedByteBuffer_insert_uint8(fbbIn, indexIn+1, tmp) ) return false;

		tmp = ((uint8_t*)&valueIn)[1];
		if( !cxa_fixedByteBuffer_insert_uint8(fbbIn, indexIn+2, tmp) ) return false;

		tmp = ((uint8_t*)&valueIn)[0];
		if( !cxa_fixedByteBuffer_insert_uint8(fbbIn, indexIn+3, tmp) ) return false;

		// increment the starting indices of our subBuffers
		incrementSubBufferStartIndices(fbbIn, true, 4);

		return true;
	}

	// if we made it here, little more complicated now since we have a parent...
	return cxa_fixedByteBuffer_insert_floatLE(fbbIn->parent, (indexIn + fbbIn->subBuff_startIndexInParent), valueIn);
}


void cxa_fixedByteBuffer_clear(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);

	// depends on whether or not we have a parent
	if( fbbIn->parent == NULL )
	{
		// we do not have a parent...
		cxa_array_clear(&fbbIn->bytes);
	}

	// if we made it here, little more complicated now since we have a parent...
	cxa_fixedByteBuffer_clear(fbbIn);
}


bool cxa_fixedByteBuffer_writeToFile_bytes(cxa_fixedByteBuffer_t *const fbbIn, FILE *fileIn)
{
	cxa_assert(fbbIn);
	cxa_assert(fileIn);
	
	for( size_t i = 0; i < cxa_fixedByteBuffer_getCurrSize(fbbIn); i++ )
	{
		if( fputc(cxa_fixedByteBuffer_get_uint8(fbbIn, i), fileIn) == EOF ) return false;
	}
	
	return true;
}

void cxa_fixedByteBuffer_writeToFile_asciiHexRep(cxa_fixedByteBuffer_t *const fbbIn, FILE *fileIn)
{
	cxa_assert(fbbIn);
	cxa_assert(fileIn);
	
	fprintf(fileIn, "fixedByteBuffer @ %p { ", fbbIn);
	for( size_t i = 0; i < cxa_fixedByteBuffer_getCurrSize(fbbIn); i++ )
	{
		fprintf(fileIn, "%02X", cxa_fixedByteBuffer_get_uint8(fbbIn, i));
		
		if( i != (cxa_fixedByteBuffer_getCurrSize(fbbIn)-1)) fputs(" ", fileIn);
	}
	fputs(" }\r\n", fileIn);
}


// ******** local function implementations ********
static void incrementSubBufferStartIndices(cxa_fixedByteBuffer_t *const fbbIn, bool isRootBufferIn, const size_t amountToIncrementIn)
{
	cxa_assert(fbbIn);

	// do our increment
	if( !isRootBufferIn ) fbbIn->subBuff_startIndexInParent += amountToIncrementIn;

	// now the same for any of our subBuffers
	for( size_t i = 0; i < cxa_array_getSize_elems(&fbbIn->subBuffers); i++ )
	{
		cxa_fixedByteBuffer_t** currSubBuffer = (cxa_fixedByteBuffer_t**)cxa_array_getAtIndex(&fbbIn->subBuffers, i);
		if( currSubBuffer == NULL ) continue;

		incrementSubBufferStartIndices(*currSubBuffer, false, amountToIncrementIn);
	}
}
