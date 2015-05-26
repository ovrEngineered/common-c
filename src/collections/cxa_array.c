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
#include <cxa_assert.h>


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
	memcpy((void*)(arrIn->bufferLoc + (arrIn->insertIndex * arrIn->datatypeSize_bytes)), itemLocIn, arrIn->datatypeSize_bytes);
	arrIn->insertIndex++;

	// if we made it here, everything was successful
	return true;
}


void* cxa_array_append_empty(cxa_array_t *const arrIn)
{
	cxa_assert(arrIn);
	
	// make sure we have enough space in the array
	if( arrIn->insertIndex >= arrIn->maxNumElements ) return NULL;
	
	void *retVal = (void*)(arrIn->bufferLoc + (arrIn->insertIndex * arrIn->datatypeSize_bytes));
	arrIn->insertIndex++;
	
	return retVal;
}


bool cxa_array_removeElement(cxa_array_t *const arrIn, const size_t indexIn)
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
	void *dest = (void*)(arrIn->bufferLoc + (indexIn * arrIn->datatypeSize_bytes));
	void *src = (void*)(arrIn->bufferLoc + (indexIn+1 * arrIn->datatypeSize_bytes));
	
	memmove(dest, src, ((arrIn->insertIndex-(indexIn+1)) * arrIn->datatypeSize_bytes));
	arrIn->insertIndex--;
	
	return true;
}


void* cxa_array_getAtIndex(cxa_array_t *const arrIn, const size_t indexIn)
{
	cxa_assert(arrIn);

	// make sure we're not out of bounds
	if( indexIn >= arrIn->insertIndex ) return NULL;

	// if we made it here, we're good to go
	return (void*)(arrIn->bufferLoc + (indexIn * arrIn->datatypeSize_bytes));
}


void *cxa_array_getAtIndex_noBoundsCheck(cxa_array_t *const arrIn, const size_t indexIn)
{
	cxa_assert(arrIn);

	// make sure we're not out of bounds
	if( indexIn >= arrIn->maxNumElements ) return NULL;

	// if we made it here, we're good to go
	return (void*)(arrIn->bufferLoc + (indexIn * arrIn->datatypeSize_bytes));	
}


const size_t cxa_array_getSize_elems(cxa_array_t *const arrIn)
{
	cxa_assert(arrIn);

	return arrIn->insertIndex;
}


const size_t cxa_array_getMaxSize_elems(cxa_array_t *const arrIn)
{
	cxa_assert(arrIn);
	
	return arrIn->maxNumElements;
}


const bool cxa_array_isFull(cxa_array_t *const arrIn)
{
	cxa_assert(arrIn);
	
	return (arrIn->insertIndex >= arrIn->maxNumElements);
}


const bool cxa_array_isEmpty(cxa_array_t *const arrIn)
{
	cxa_assert(arrIn);
	
	return (arrIn->insertIndex == 0);
}


void cxa_array_clear(cxa_array_t *const arrIn)
{
	cxa_assert(arrIn);
	
	arrIn->insertIndex = 0;
}


size_t cxa_array_getFreeSize_elems(cxa_array_t *const arrIn)
{
	cxa_assert(arrIn);
	
	return (arrIn->maxNumElements - arrIn->insertIndex);
}


// ******** local function implementations ********

