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
	
	// set our type and parent
	fbbIn->initType = CXA_FBB_INITTYPE_NORMAL;
	fbbIn->parent = NULL;
}


void cxa_fixedByteBuffer_init_subsetOfData(cxa_fixedByteBuffer_t *const subsetFbbIn, cxa_fixedByteBuffer_t *const sourceFbbIn,
	size_t startIndexIn, ssize_t length_BytesIn)
{
	cxa_assert(subsetFbbIn);
	cxa_assert(sourceFbbIn);
	cxa_assert(startIndexIn < cxa_fixedByteBuffer_getCurrSize(sourceFbbIn) );

	// make sure we enough data in the source buffer
	if( length_BytesIn != CXA_FIXED_BYTE_BUFFER_LEN_ALL )
	{
		cxa_assert( (startIndexIn + length_BytesIn) <= cxa_fixedByteBuffer_getCurrSize(sourceFbbIn) );
	}
	else length_BytesIn = cxa_fixedByteBuffer_getCurrSize(sourceFbbIn) - startIndexIn;
	
	// we're good to go, setup our subset byte buffer
	cxa_array_init_inPlace(&subsetFbbIn->bytes, 1,
		length_BytesIn,
		(void*)cxa_fixedByteBuffer_getAtIndex(sourceFbbIn, startIndexIn),
		length_BytesIn);
		
	// set our type
	subsetFbbIn->initType = CXA_FBB_INITTYPE_SUBSET_DATA;
	subsetFbbIn->parent = sourceFbbIn;
	subsetFbbIn->startIndexInParent = startIndexIn;
}


void cxa_fixedByteBuffer_init_subsetOfCapacity(cxa_fixedByteBuffer_t *const subsetFbbIn, cxa_fixedByteBuffer_t *const sourceFbbIn,
	size_t startIndexIn, ssize_t length_BytesIn)
{
	cxa_assert(subsetFbbIn);
	cxa_assert(sourceFbbIn);

	size_t srcMaxSize_bytes = cxa_fixedByteBuffer_getMaxSize(sourceFbbIn);
	cxa_assert(startIndexIn < srcMaxSize_bytes );

	// make sure we enough data in the source buffer
	if( length_BytesIn != CXA_FIXED_BYTE_BUFFER_LEN_ALL )
	{
		cxa_assert( (startIndexIn + length_BytesIn) <= srcMaxSize_bytes );
	}
	else length_BytesIn = srcMaxSize_bytes - startIndexIn;
	
	// calculate the size of the new subset buffer
	size_t srcCurrSize_bytes = cxa_fixedByteBuffer_getCurrSize(sourceFbbIn);
	size_t subsetCurrSize_bytes = (srcCurrSize_bytes < startIndexIn) ? 0 : (srcCurrSize_bytes - startIndexIn);
		
	// we're good to go, setup our subset byte buffer
	cxa_array_init_inPlace(&subsetFbbIn->bytes, 1,
		subsetCurrSize_bytes,
		(void*)cxa_array_getAtIndex_noBoundsCheck(&sourceFbbIn->bytes, startIndexIn),
		length_BytesIn);
		
	// set our type
	subsetFbbIn->initType = CXA_FBB_INITTYPE_SUBSET_CAP;
	subsetFbbIn->parent = sourceFbbIn;
	subsetFbbIn->startIndexInParent = startIndexIn;
}


size_t cxa_fixedByteBuffer_getCurrSize(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);
	return cxa_array_getSize_elems(&fbbIn->bytes);	
}


size_t cxa_fixedByteBuffer_getMaxSize(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);
	return cxa_array_getMaxSize_elems(&fbbIn->bytes);
}


bool cxa_fixedByteBuffer_isEmpty(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);
	return cxa_array_isEmpty(&fbbIn->bytes);
}


bool cxa_fixedByteBuffer_isFull(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);
	return cxa_array_isFull(&fbbIn->bytes);	
}


bool cxa_fixedByteBuffer_append_byte(cxa_fixedByteBuffer_t *const fbbIn, uint8_t byteIn)
{
	cxa_assert(fbbIn);
	bool retVal = cxa_array_append(&fbbIn->bytes, &byteIn);
	
	// see if we need to adjust our parent's size to match our new addition
	if( retVal &&
		(fbbIn->parent != NULL) &&
		(fbbIn->startIndexInParent <= cxa_fixedByteBuffer_getCurrSize(fbbIn->parent)) &&
		((fbbIn->startIndexInParent + cxa_fixedByteBuffer_getCurrSize(fbbIn)) == (cxa_fixedByteBuffer_getCurrSize(fbbIn->parent)+1)) )
	{
		cxa_array_append_empty(&fbbIn->parent->bytes);
	}
	
	return retVal;
}


bool cxa_fixedByteBuffer_append_uint16LE(cxa_fixedByteBuffer_t *const fbbIn, uint16_t uint16In)
{
	cxa_assert(fbbIn);
	
	uint8_t tmp = (uint8_t)((uint16In & (uint16_t)0x00FF) >> 0);
	if( !cxa_fixedByteBuffer_append_byte(fbbIn, tmp) ) return false;
	
	tmp = (uint8_t)((uint16In & (uint16_t)0xFF00) >> 8);
	if( !cxa_fixedByteBuffer_append_byte(fbbIn, tmp) ) return false;
	
	return true;
}


bool cxa_fixedByteBuffer_append_uint32LE(cxa_fixedByteBuffer_t *const fbbIn, uint32_t uint32In)
{
	cxa_assert(fbbIn);
		
	uint8_t tmp = (uint8_t)((uint32In & (uint32_t)0x000000FF) >> 0);
	if( !cxa_fixedByteBuffer_append_byte(fbbIn, tmp) ) return false;
		
	tmp = (uint8_t)((uint32In & (uint32_t)0x0000FF00) >> 8);
	if( !cxa_fixedByteBuffer_append_byte(fbbIn, tmp) ) return false;
	
	tmp = (uint8_t)((uint32In & (uint32_t)0x00FF0000) >> 16);
	if( !cxa_fixedByteBuffer_append_byte(fbbIn, tmp) ) return false;
		
	tmp = (uint8_t)((uint32In & (uint32_t)0xFF000000) >> 24);
	if( !cxa_fixedByteBuffer_append_byte(fbbIn, tmp) ) return false;
	
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
		if( !cxa_fixedByteBuffer_append_byte(fbbIn, *cxa_fixedByteBuffer_getAtIndex(srcFbbIn, i)) ) return false;
	}
	return true;
}


uint8_t* cxa_fixedByteBuffer_getAtIndex(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn)
{
	cxa_assert(fbbIn);
	return (uint8_t*)cxa_array_getAtIndex(&fbbIn->bytes, indexIn);
}


uint16_t cxa_fixedByteBuffer_getUint16_LE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn)
{
	cxa_assert(fbbIn);
	cxa_assert( (indexIn >= 0) && ((indexIn + 2) <= cxa_array_getSize_elems(&fbbIn->bytes)) );
	
	uint8_t* dataPtr = (uint8_t*)cxa_array_getAtIndex(&fbbIn->bytes, indexIn);
	cxa_assert(dataPtr);
	
	return (((uint16_t)dataPtr[0]) <<  0) | (((uint16_t)dataPtr[1]) <<  8);
}


uint32_t cxa_fixedByteBuffer_getUint32_LE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn)
{
	cxa_assert(fbbIn);
	cxa_assert( (indexIn >= 0) && ((indexIn + 4) <= cxa_array_getSize_elems(&fbbIn->bytes)) );
	
	uint8_t* dataPtr = cxa_array_getAtIndex(&fbbIn->bytes, indexIn);
	cxa_assert(dataPtr);
	
	return (((uint32_t)dataPtr[0]) <<  0) | (((uint32_t)dataPtr[1]) <<  8) |
		   (((uint32_t)dataPtr[2]) << 16) | (((uint32_t)dataPtr[3]) << 24);
}


float cxa_fixedByteBuffer_getFloat_LE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn)
{
	cxa_assert(fbbIn);
	cxa_assert( (indexIn >= 0) && ((indexIn + 4) <= cxa_array_getSize_elems(&fbbIn->bytes)) );
	
	uint32_t intVal = cxa_fixedByteBuffer_getUint32_LE(fbbIn, indexIn);
	float retVal = 0;
	((uint8_t*)&retVal)[0] = ((uint8_t*)&intVal)[0];
	((uint8_t*)&retVal)[1] = ((uint8_t*)&intVal)[1];
	((uint8_t*)&retVal)[2] = ((uint8_t*)&intVal)[2];
	((uint8_t*)&retVal)[3] = ((uint8_t*)&intVal)[3];
	
	return retVal;
}


void cxa_fixedByteBuffer_clear(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);
	
	// we have some unique functionality here...
	if( fbbIn->initType == CXA_FBB_INITTYPE_NORMAL ) cxa_array_clear(&fbbIn->bytes);
	else
	{
		// if we're in sync with the parent (ie. same uppper size boundary) we'll also shrink
		// the parent appropriately
		if( (fbbIn->startIndexInParent + cxa_fixedByteBuffer_getCurrSize(fbbIn)) == cxa_fixedByteBuffer_getCurrSize(fbbIn->parent) )
		{
			// same upper boundary...shrink our parent by our size
			size_t mySize_elems = cxa_fixedByteBuffer_getCurrSize(fbbIn);
			for( size_t i = 0; i < mySize_elems; i++ )
			{
				cxa_array_removeElement(&fbbIn->parent->bytes, cxa_fixedByteBuffer_getCurrSize(fbbIn->parent)-1);
			}
			
			// now that our parent has been shrunk, set our size to 0
			cxa_array_clear(&fbbIn->bytes);
		}
		else
		{
			// we are out of sync with the parent (they have data "after" us in their array)
			// or we are in the parent's "free space"...don't modify the parent, just reset our size
			cxa_array_clear(&fbbIn->bytes);
		}
	}
}


size_t cxa_fixedByteBuffer_getBytesRemaining(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);
	return cxa_array_getFreeSize_elems(&fbbIn->bytes);
}


bool cxa_fixedByteBuffer_writeToFile_bytes(cxa_fixedByteBuffer_t *const fbbIn, FILE *fileIn)
{
	cxa_assert(fbbIn);
	cxa_assert(fileIn);
	
	for( size_t i = 0; i < cxa_array_getSize_elems(&fbbIn->bytes); i++ )
	{
		if( fputc(*cxa_fixedByteBuffer_getAtIndex(fbbIn, i), fileIn) == EOF ) return false;
	}
	
	return true;
}

void cxa_fixedByteBuffer_writeToFile_asciiHexRep(cxa_fixedByteBuffer_t *const fbbIn, FILE *fileIn)
{
	cxa_assert(fbbIn);
	cxa_assert(fileIn);
	
	fprintf(fileIn, "fixedByteBuffer @ %p { ", fbbIn);
	for( size_t i = 0; i < cxa_array_getSize_elems(&fbbIn->bytes); i++ )
	{
		fprintf(fileIn, "%02X", *cxa_fixedByteBuffer_getAtIndex(fbbIn, i));
		
		if( i != (cxa_array_getSize_elems(&fbbIn->bytes)-1)) fputs(" ", fileIn);
	}
	fputs(" }\r\n", fileIn);
}


// ******** local function implementations ********

