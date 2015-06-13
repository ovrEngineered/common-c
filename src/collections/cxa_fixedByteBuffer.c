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
static void incrementSubBufferStartIndices(cxa_array_t* subBuffersIn, const size_t indexOfChangeIn, const size_t amountToIncrementIn);
static void decrementSubBufferStartIndices(cxa_array_t* subBuffersIn, const size_t indexOfChangeIn, const size_t amountToDecrementIn);
static bool removeNumberOfBytes(cxa_fixedByteBuffer_t *const fbbIn, size_t indexIn, size_t numBytesIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_fixedByteBuffer_init(cxa_fixedByteBuffer_t *const fbbIn, void *const bufferLocIn, const size_t bufferMaxSize_bytesIn)
{
	cxa_assert(fbbIn);
	cxa_assert(bufferLocIn);
	cxa_assert(bufferMaxSize_bytesIn > 0);
	
	// setup our internal state
	cxa_array_init(&fbbIn->bytes, 1, bufferLocIn, bufferMaxSize_bytesIn);
	cxa_array_initStd(&fbbIn->subBuffers, fbbIn->subBuffers_raw);
	
	// we have no parent
	fbbIn->parent = NULL;
}


bool cxa_fixedByteBuffer_init_subBuffer(cxa_fixedByteBuffer_t *const subFbbIn, cxa_fixedByteBuffer_t *const parentFbbIn,
		size_t startIndexIn, size_t maxSize_bytesIn)
{
	cxa_assert(subFbbIn);
	cxa_assert(parentFbbIn);

	size_t parentMaxSize_bytes = cxa_fixedByteBuffer_getMaxSize_bytes(parentFbbIn);

	// make sure our sizes / indices are appropriate
	if( maxSize_bytesIn == CXA_FIXED_BYTE_BUFFER_LEN_ALL )
	{
		if( startIndexIn >= parentMaxSize_bytes ) return false;
	}
	else
	{
		if( (startIndexIn + maxSize_bytesIn) > parentMaxSize_bytes ) return false;
	}

	// setup our internal state
	cxa_array_initStd(&subFbbIn->subBuffers, subFbbIn->subBuffers_raw);

	// store our references
	subFbbIn->parent = parentFbbIn;
	cxa_array_append(&parentFbbIn->subBuffers, (void*)&subFbbIn);
	subFbbIn->subBuff_startIndexInParent = startIndexIn;

	subFbbIn->subBuff_hasMaxSize = (maxSize_bytesIn != CXA_FIXED_BYTE_BUFFER_LEN_ALL);
	subFbbIn->subBuff_maxSize_bytes = (subFbbIn->subBuff_hasMaxSize) ? maxSize_bytesIn : 0;

	return true;
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

	// we can't append until the parent is filled to our index
	size_t parentSize_bytes = cxa_fixedByteBuffer_getSize_bytes(fbbIn->parent);
	if( parentSize_bytes < fbbIn->subBuff_startIndexInParent ) return false;

	// we can't append if we are full
	if( cxa_fixedByteBuffer_getFreeSize_bytes(fbbIn) < 1 ) return false;

	return cxa_fixedByteBuffer_append_uint8(fbbIn->parent, byteIn);
}


bool cxa_fixedByteBuffer_append_uint16LE(cxa_fixedByteBuffer_t *const fbbIn, uint16_t uint16In)
{
	cxa_assert(fbbIn);
	
	// make sure we have room for the operation
	if( cxa_fixedByteBuffer_getFreeSize_bytes(fbbIn) < 2 ) return false;

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
	if( cxa_fixedByteBuffer_getFreeSize_bytes(fbbIn) < 4 ) return false;
		
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
	if( cxa_fixedByteBuffer_getFreeSize_bytes(fbbIn) < 4 ) return false;

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

	size_t stringSize_bytes = strlen(stringIn)+1;

	// make sure we have room for the operation (string + null-term)
	if( cxa_fixedByteBuffer_getFreeSize_bytes(fbbIn) < stringSize_bytes ) return false;

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
	
	size_t numBytes_src = cxa_fixedByteBuffer_getSize_bytes(srcFbbIn);
	if( numBytes_src > cxa_fixedByteBuffer_getFreeSize_bytes(fbbIn) ) return false;
	
	for( size_t i = 0; i < numBytes_src; i++ )
	{
		uint8_t tmpByte;
		if( !cxa_fixedByteBuffer_get_uint8(srcFbbIn, i, &tmpByte) ) return false;

		if( !cxa_fixedByteBuffer_append_uint8(fbbIn, tmpByte) ) return false;
	}
	return true;
}


bool cxa_fixedByteBuffer_remove_uint8(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn)
{
	cxa_assert(fbbIn);

	// depends on whether or not we have a parent
	if( fbbIn->parent == NULL )
	{
		// we do not have a parent...
		if( !cxa_array_remove(&fbbIn->bytes, indexIn) ) return false;

		// decrement the starting indices of our subBuffers
		decrementSubBufferStartIndices(&fbbIn->subBuffers, indexIn, 1);

		return true;
	}

	// if we made it here, little more complicated now since we have a parent...
	return cxa_fixedByteBuffer_remove_uint8(fbbIn->parent, fbbIn->subBuff_startIndexInParent+indexIn);
}


bool cxa_fixedByteBuffer_remove_uint16LE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn)
{
	cxa_assert(fbbIn);
	return removeNumberOfBytes(fbbIn, indexIn, 2);
}


bool cxa_fixedByteBuffer_remove_uint32LE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn)
{
	cxa_assert(fbbIn);
	return removeNumberOfBytes(fbbIn, indexIn, 4);
}


bool cxa_fixedByteBuffer_remove_uintFloatLE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn)
{
	cxa_assert(fbbIn);
	return removeNumberOfBytes(fbbIn, indexIn, 4);
}


bool cxa_fixedByteBuffer_remove_cString(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn)
{
	cxa_assert(fbbIn);

	uint8_t* targetString = cxa_fixedByteBuffer_get_pointerToIndex(fbbIn, indexIn);
	if( targetString == NULL ) return false;

	// figure out how long the string is...
	size_t strLen_bytes = strlen((const char*)targetString) + 1;

	return removeNumberOfBytes(fbbIn, indexIn, strLen_bytes);
}


uint8_t* cxa_fixedByteBuffer_get_pointerToIndex(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn)
{
	cxa_assert(fbbIn);

	// depends on whether or not we have a parent
	if( fbbIn->parent == NULL )
	{
		// we do not have a parent...
		return (uint8_t*)cxa_array_get(&fbbIn->bytes, indexIn);
	}

	// if we made it here, little more complicated now since we have a parent...
	return cxa_fixedByteBuffer_get_pointerToIndex(fbbIn->parent, indexIn + fbbIn->subBuff_startIndexInParent);
}


bool cxa_fixedByteBuffer_get_uint8(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, uint8_t* valOut)
{
	uint8_t* retVal = cxa_fixedByteBuffer_get_pointerToIndex(fbbIn, indexIn);

	if( !retVal ) return false;

	if( valOut ) *valOut = *retVal;
	return true;
}


bool cxa_fixedByteBuffer_get_uint16LE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, uint16_t* valOut)
{
	cxa_assert(fbbIn);
	
	uint8_t* byte0 = cxa_fixedByteBuffer_get_pointerToIndex(fbbIn, indexIn);
	uint8_t* byte1 = cxa_fixedByteBuffer_get_pointerToIndex(fbbIn, indexIn+1);
	
	if( !byte0 || !byte1 ) return false;

	if( valOut ) *valOut = (((uint16_t)*byte0) <<  0) | (((uint16_t)*byte1) <<  8);
	return true;
}


bool cxa_fixedByteBuffer_get_uint32LE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, uint32_t* valOut)
{
	cxa_assert(fbbIn);
	
	uint8_t* byte0 = cxa_fixedByteBuffer_get_pointerToIndex(fbbIn, indexIn);
	uint8_t* byte1 = cxa_fixedByteBuffer_get_pointerToIndex(fbbIn, indexIn+1);
	uint8_t* byte2 = cxa_fixedByteBuffer_get_pointerToIndex(fbbIn, indexIn+2);
	uint8_t* byte3 = cxa_fixedByteBuffer_get_pointerToIndex(fbbIn, indexIn+3);
	
	if( !byte0 || !byte1 || !byte2 || !byte3 ) return false;

	if( valOut ) *valOut = (((uint32_t)*byte0) <<  0) | (((uint32_t)*byte1) <<  8) | (((uint32_t)*byte2) << 16) | (((uint32_t)*byte3) << 24);
	return true;
}


bool cxa_fixedByteBuffer_get_floatLE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, float* valOut)
{
	cxa_assert(fbbIn);

	uint32_t intVal = 0;
	if( !cxa_fixedByteBuffer_get_uint32LE(fbbIn, indexIn, &intVal) ) return false;

	float retVal = 0;
	((uint8_t*)&retVal)[0] = ((uint8_t*)&intVal)[0];
	((uint8_t*)&retVal)[1] = ((uint8_t*)&intVal)[1];
	((uint8_t*)&retVal)[2] = ((uint8_t*)&intVal)[2];
	((uint8_t*)&retVal)[3] = ((uint8_t*)&intVal)[3];
	if( valOut ) *valOut = retVal;
	
	return true;
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
		return cxa_array_overwrite(&fbbIn->bytes, indexIn, &byteIn);
	}

	// if we made it here, little more complicated now since we have a parent...
	return cxa_fixedByteBuffer_overwrite_uint8(fbbIn->parent, (indexIn + fbbIn->subBuff_startIndexInParent), byteIn);
}


bool cxa_fixedByteBuffer_overwrite_uint16LE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, uint16_t uint16In)
{
	cxa_assert(fbbIn);

	// make sure we have room for this operation at the specified index
	if( (indexIn+2) > cxa_fixedByteBuffer_getSize_bytes(fbbIn) ) return false;

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
	if( (indexIn+4) > cxa_fixedByteBuffer_getSize_bytes(fbbIn) ) return false;

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
	if( (indexIn+4) > cxa_fixedByteBuffer_getSize_bytes(fbbIn) ) return false;

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


bool cxa_fixedByteBuffer_insert_uint8(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, uint8_t valueIn)
{
	cxa_assert(fbbIn);

	// depends on whether or not we have a parent
	if( fbbIn->parent == NULL )
	{
		// we do not have a parent...do our insert
		if( !cxa_array_insert(&fbbIn->bytes, indexIn, &valueIn) ) return false;

		// increment the starting indices of our subBuffers
		incrementSubBufferStartIndices(&fbbIn->subBuffers, indexIn, 1);

		return true;
	}

	// if we made it here, little more complicated now since we have a parent...
	return cxa_fixedByteBuffer_insert_uint8(fbbIn->parent, (indexIn + fbbIn->subBuff_startIndexInParent), valueIn);
}


bool cxa_fixedByteBuffer_insert_uint16LE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, uint16_t valueIn)
{
	cxa_assert(fbbIn);

	// make sure we have room for the operation
	if( cxa_fixedByteBuffer_getFreeSize_bytes(fbbIn) < 2 ) return false;

	uint8_t byte0 = (uint8_t)((valueIn & (uint16_t)0x00FF) >> 0);
	if( !cxa_fixedByteBuffer_insert_uint8(fbbIn, indexIn, byte0) ) return false;
	uint8_t byte1 = (uint8_t)((valueIn & (uint16_t)0xFF00) >> 8);
	if( !cxa_fixedByteBuffer_insert_uint8(fbbIn, indexIn+1, byte1) ) return false;

	return true;
}


bool cxa_fixedByteBuffer_insert_uint32LE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, uint32_t valueIn)
{
	cxa_assert(fbbIn);

	// make sure we have room for the operation
	if( cxa_fixedByteBuffer_getFreeSize_bytes(fbbIn) < 4 ) return false;

	// we do not have a parent...do our insert
	uint8_t byte0 = (uint8_t)((valueIn & (uint32_t)0x000000FF) >> 0);
	if( !cxa_fixedByteBuffer_insert_uint8(fbbIn, indexIn, byte0) ) return false;

	uint8_t byte1 = (uint8_t)((valueIn & (uint32_t)0x0000FF00) >> 8);
	if( !cxa_fixedByteBuffer_insert_uint8(fbbIn, indexIn+1, byte1) ) return false;

	uint8_t byte2 = (uint8_t)((valueIn & (uint32_t)0x00FF0000) >> 16);
	if( !cxa_fixedByteBuffer_insert_uint8(fbbIn, indexIn+2, byte2) ) return false;

	uint8_t byte3 = (uint8_t)((valueIn & (uint32_t)0xFF000000) >> 24);
	if( !cxa_fixedByteBuffer_insert_uint8(fbbIn, indexIn+3, byte3) ) return false;

	return true;
}


bool cxa_fixedByteBuffer_insert_floatLE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, float valueIn)
{
	cxa_assert(fbbIn);

	uint32_t intVal = *((uint32_t*)&valueIn);
	return cxa_fixedByteBuffer_insert_uint32LE(fbbIn, indexIn, intVal);
}


bool cxa_fixedByteBuffer_insert_cString(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, char *const stringIn)
{
	cxa_assert(fbbIn);
	cxa_assert(stringIn);

	size_t stringSize_bytes = strlen(stringIn)+1;

	// make sure we have room for the operation (string + null-term)
	if( cxa_fixedByteBuffer_getFreeSize_bytes(fbbIn) < stringSize_bytes ) return false;

	for( size_t i = 0; i < stringSize_bytes; i++ )
	{
		if( !cxa_fixedByteBuffer_insert_uint8(fbbIn, indexIn+i, (uint8_t)stringIn[i]) ) return false;
	}

	return true;
}


size_t cxa_fixedByteBuffer_getSize_bytes(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);

	// depends on whether or not we have a parent
	if( fbbIn->parent == NULL )
	{
		return cxa_array_getSize_elems(&fbbIn->bytes);
	}

	size_t parentSize_bytes = cxa_fixedByteBuffer_getSize_bytes(fbbIn->parent);

	return (parentSize_bytes < fbbIn->subBuff_startIndexInParent) ? 0 : (parentSize_bytes - fbbIn->subBuff_startIndexInParent);
}


size_t cxa_fixedByteBuffer_getMaxSize_bytes(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);

	// depends on whether or not we have a parent
	if( fbbIn->parent == NULL )
	{
		return cxa_array_getMaxSize_elems(&fbbIn->bytes);
	}

	// if we made it here, little more complicated now since we have a parent...
	return fbbIn->subBuff_hasMaxSize ? fbbIn->subBuff_maxSize_bytes : (cxa_fixedByteBuffer_getMaxSize_bytes(fbbIn->parent) - fbbIn->subBuff_startIndexInParent);
}


size_t cxa_fixedByteBuffer_getFreeSize_bytes(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);

	// depends on whether or not we have a parent
	if( fbbIn->parent == NULL )
	{
		return cxa_array_getFreeSize_elems(&fbbIn->bytes);
	}

	// if we made it here, little more complicated now since we have a parent...
	return cxa_fixedByteBuffer_getMaxSize_bytes(fbbIn) - cxa_fixedByteBuffer_getSize_bytes(fbbIn);
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

	// depends on whether or not we have a parent
	if( fbbIn->parent == NULL )
	{
		// we do not have a parent...
		cxa_array_clear(&fbbIn->bytes);
		return;
	}

	// if we made it here, little more complicated now since we have a parent...
	cxa_fixedByteBuffer_clear(fbbIn->parent);
}


bool cxa_fixedByteBuffer_writeToFile_bytes(cxa_fixedByteBuffer_t *const fbbIn, FILE *fileIn)
{
	cxa_assert(fbbIn);
	cxa_assert(fileIn);
	
	for( size_t i = 0; i < cxa_fixedByteBuffer_getSize_bytes(fbbIn); i++ )
	{
		uint8_t currByte;
		if( !cxa_fixedByteBuffer_get_uint8(fbbIn, i, &currByte) ) return false;

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
		if( !cxa_fixedByteBuffer_get_uint8(fbbIn, i, &currByte) ) return false;

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
static void incrementSubBufferStartIndices(cxa_array_t* subBuffersIn, const size_t indexOfChangeIn, const size_t amountToIncrementIn)
{
	cxa_assert(subBuffersIn);

	// iterate over our subBuffers
	cxa_array_iterate(subBuffersIn, currSubBuffer, cxa_fixedByteBuffer_t*)
	{
		if( indexOfChangeIn < (*currSubBuffer)->subBuff_startIndexInParent )
		{
			(*currSubBuffer)->subBuff_startIndexInParent += amountToIncrementIn;
			incrementSubBufferStartIndices(&(*currSubBuffer)->subBuffers, indexOfChangeIn, amountToIncrementIn);
		}
	}
}


static void decrementSubBufferStartIndices(cxa_array_t* subBuffersIn, const size_t indexOfChangeIn, const size_t amountToDecrementIn)
{
	cxa_assert(subBuffersIn);

	// iterate over our subBuffers
	cxa_array_iterate(subBuffersIn, currSubBuffer, cxa_fixedByteBuffer_t*)
	{
		if( indexOfChangeIn < (*currSubBuffer)->subBuff_startIndexInParent )
		{
			(*currSubBuffer)->subBuff_startIndexInParent -= amountToDecrementIn;
			decrementSubBufferStartIndices(&(*currSubBuffer)->subBuffers, indexOfChangeIn, amountToDecrementIn);
		}
	}
}


static bool removeNumberOfBytes(cxa_fixedByteBuffer_t *const fbbIn, size_t indexIn, size_t numBytesIn)
{
	cxa_assert(fbbIn);

	// make sure we have enough bytes at this index to remove
	if( (indexIn+numBytesIn) > cxa_fixedByteBuffer_getSize_bytes(fbbIn) ) return false;

	// remove the bytes
	for( size_t i = 0; i < numBytesIn; i++ )
	{
		if( !cxa_fixedByteBuffer_remove_uint8(fbbIn, indexIn) ) return false;
	}

	return true;
}
