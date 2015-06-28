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
#include "cxa_array.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <string.h>
#include <stdint.h>
#include <cxa_assert.h>
#include <cxa_config.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_array_init(cxa_array_t *const arrIn, const size_t datatypeSize_bytesIn, void *const bufferLocIn, const size_t bufferMaxSize_bytesIn)
{
	cxa_assert(arrIn);
	cxa_assert(datatypeSize_bytesIn > 0);
	cxa_assert(bufferLocIn);
	cxa_assert(bufferMaxSize_bytesIn >= datatypeSize_bytesIn);

	// save our references
	arrIn->bufferLoc = bufferLocIn;
	arrIn->datatypeSize_bytes = datatypeSize_bytesIn;
	arrIn->maxNumElements = bufferMaxSize_bytesIn / datatypeSize_bytesIn;

	// set some reasonable defaults
	arrIn->insertIndex = 0;
}


void cxa_array_init_inPlace(cxa_array_t *const arrIn, const size_t datatypeSize_bytesIn, const size_t currNumElemsIn, void *const bufferLocIn, const size_t bufferMaxSize_bytesIn)
{
	cxa_assert(arrIn);
	cxa_assert(datatypeSize_bytesIn > 0);
	cxa_assert(bufferLocIn);
	cxa_assert( (currNumElemsIn*datatypeSize_bytesIn) <= bufferMaxSize_bytesIn );
	
	// save our references
	arrIn->bufferLoc = bufferLocIn;
	arrIn->datatypeSize_bytes = datatypeSize_bytesIn;
	arrIn->maxNumElements = bufferMaxSize_bytesIn / datatypeSize_bytesIn;
	
	// set our size
	arrIn->insertIndex = currNumElemsIn;
}


bool cxa_array_append(cxa_array_t *const arrIn, void *const itemLocIn)
{
	cxa_assert(arrIn);
	cxa_assert(itemLocIn);

	// make sure we have enough space in the array
	if( arrIn->insertIndex >= arrIn->maxNumElements ) return false;
	
	// if we made it here, we have enough elements, get ready to copy the item
	memcpy((void*)(((uint8_t*)arrIn->bufferLoc) + (arrIn->insertIndex * arrIn->datatypeSize_bytes)), itemLocIn, arrIn->datatypeSize_bytes);
	arrIn->insertIndex++;

	// if we made it here, everything was successful
	return true;
}


void* cxa_array_append_empty(cxa_array_t *const arrIn)
{
	cxa_assert(arrIn);
	
	// make sure we have enough space in the array
	if( arrIn->insertIndex >= arrIn->maxNumElements ) return NULL;
	
	void *retVal = (void*)(((uint8_t*)arrIn->bufferLoc) + (arrIn->insertIndex * arrIn->datatypeSize_bytes));
	arrIn->insertIndex++;
	
	return retVal;
}


bool cxa_array_remove_atIndex(cxa_array_t *const arrIn, const size_t indexIn)
{
	cxa_assert(arrIn);
	
	// make sure we're not out of bounds
	if( indexIn >= arrIn->insertIndex ) return false;
	
	// if this is the last element in the array, we don't need to do any memmoves
	if( (indexIn+1) >= arrIn->insertIndex )
	{
		arrIn->insertIndex--;
		return true;
	}	
	
	// if we made it here, we have some data to move around
	void *dest = (void*)(((uint8_t*)arrIn->bufferLoc) + (indexIn * arrIn->datatypeSize_bytes));
	void *src = (void*)(((uint8_t*)arrIn->bufferLoc) + (indexIn+1 * arrIn->datatypeSize_bytes));
	
	memmove(dest, src, ((arrIn->insertIndex-(indexIn+1)) * arrIn->datatypeSize_bytes));
	arrIn->insertIndex--;
	
	return true;
}


bool cxa_array_remove(cxa_array_t *const arrIn, void *const itemLocIn)
{
	cxa_assert(arrIn);
	if( itemLocIn == NULL ) return false;

	for( size_t i = 0; i < cxa_array_getSize_elems(arrIn); i++ )
	{
		if( cxa_array_get(arrIn, i) == itemLocIn )
		{
			return cxa_array_remove_atIndex(arrIn, i);
		}
	}

	return false;
}


void* cxa_array_get(cxa_array_t *const arrIn, const size_t indexIn)
{
	cxa_assert(arrIn);

	// make sure we're not out of bounds
	if( indexIn >= arrIn->insertIndex ) return NULL;

	// if we made it here, we're good to go
	return (void*)(((uint8_t*)arrIn->bufferLoc) + (indexIn * arrIn->datatypeSize_bytes));
}


void *cxa_array_get_noBoundsCheck(cxa_array_t *const arrIn, const size_t indexIn)
{
	cxa_assert(arrIn);

	// make sure we're not out of bounds
	if( indexIn >= arrIn->maxNumElements ) return NULL;

	// if we made it here, we're good to go
	return (void*)(((uint8_t*)arrIn->bufferLoc) + (indexIn * arrIn->datatypeSize_bytes));	
}


bool cxa_array_overwrite(cxa_array_t *const arrIn, const size_t indexIn, void *const itemLocIn)
{
	cxa_assert(arrIn);
	cxa_assert(itemLocIn);

	// make sure the index is within our current data
	if( indexIn >= cxa_array_getSize_elems(arrIn) ) return false;

	// if we made it here, get ready to copy the item
	memcpy((void*)(((uint8_t*)arrIn->bufferLoc) + (indexIn * arrIn->datatypeSize_bytes)), itemLocIn, arrIn->datatypeSize_bytes);

	// if we made it here, everything was successful
	return true;
}


bool cxa_array_insert(cxa_array_t *const arrIn, const size_t indexIn, void *const itemLocIn)
{
	cxa_assert(arrIn);
	cxa_assert(itemLocIn);

	// make sure we have enough space in the array
	size_t currSize = cxa_array_getSize_elems(arrIn);
	if( currSize == arrIn->maxNumElements ) return false;

	// make sure the index is within our current data (or just outside for appends)
	if( indexIn > cxa_array_getSize_elems(arrIn) ) return false;

	// increment our insert index (since we're adding an element);
	arrIn->insertIndex++;

	// move our other items
	memmove( (void*)(((uint8_t*)arrIn->bufferLoc) + ((indexIn+1) * arrIn->datatypeSize_bytes)),
			 (void*)(((uint8_t*)arrIn->bufferLoc) + (indexIn * arrIn->datatypeSize_bytes)),
			 (currSize-indexIn) * arrIn->datatypeSize_bytes );

	// copy in our new item
	memcpy((void*)(((uint8_t*)arrIn->bufferLoc) + (indexIn * arrIn->datatypeSize_bytes)), itemLocIn, arrIn->datatypeSize_bytes);

	return true;
}


size_t cxa_array_getSize_elems(cxa_array_t *const arrIn)
{
	cxa_assert(arrIn);

	return arrIn->insertIndex;
}


size_t cxa_array_getMaxSize_elems(cxa_array_t *const arrIn)
{
	cxa_assert(arrIn);
	
	return arrIn->maxNumElements;
}


size_t cxa_array_getFreeSize_elems(cxa_array_t *const arrIn)
{
	cxa_assert(arrIn);

	return (arrIn->maxNumElements - arrIn->insertIndex);
}


bool cxa_array_isFull(cxa_array_t *const arrIn)
{
	cxa_assert(arrIn);
	
	return (arrIn->insertIndex >= arrIn->maxNumElements);
}


bool cxa_array_isEmpty(cxa_array_t *const arrIn)
{
	cxa_assert(arrIn);
	
	return (arrIn->insertIndex == 0);
}


void cxa_array_clear(cxa_array_t *const arrIn)
{
	cxa_assert(arrIn);
	
	arrIn->insertIndex = 0;
}


bool cxa_array_writeToFile_asciiHexRep(cxa_array_t *const arrIn, FILE *fileIn)
{
	cxa_assert(arrIn);
	cxa_assert(fileIn);

	if( fprintf(fileIn, "array @ %p" CXA_LINE_ENDING "{" CXA_LINE_ENDING, arrIn) < 0 ) return false;
	for( size_t i = 0; i < cxa_array_getSize_elems(arrIn); i++ )
	{
		if( fprintf(fileIn, "   %lu::0x", i) < 0 ) return false;
		for( size_t byteOffset = 0; byteOffset < arrIn->datatypeSize_bytes; byteOffset++ )
		{
			if( fprintf(fileIn, "%02X", ((uint8_t*)arrIn->bufferLoc)[(i * arrIn->datatypeSize_bytes) + byteOffset]) < 0 ) return false;
		}
		if( fputs(CXA_LINE_ENDING, fileIn) < 0 ) return false;
	}
	if( fputs("}" CXA_LINE_ENDING, fileIn) < 0 ) return false;

	return true;
}


// ******** local function implementations ********

