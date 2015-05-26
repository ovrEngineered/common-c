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
#include "cxa_map.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <string.h>
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static bool getIndex_byKey(cxa_map_t *const mapIn, void *keyIn, size_t *indexOut);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_map_init(cxa_map_t *const mapIn, size_t keySize_bytesIn, size_t valueSize_bytesIn, void *const bufferLocIn, const size_t bufferMaxSize_bytesIn)
{
	cxa_assert(mapIn);
	cxa_assert(keySize_bytesIn > 0);
	cxa_assert(valueSize_bytesIn > 0);
	cxa_assert(bufferLocIn);
	cxa_assert(bufferMaxSize_bytesIn >= (keySize_bytesIn+valueSize_bytesIn));

	// save our references
	mapIn->bufferLoc = bufferLocIn;
	mapIn->keySize_bytes = keySize_bytesIn;
	mapIn->valueSize_bytes = valueSize_bytesIn;
	mapIn->maxNumElements = bufferMaxSize_bytesIn / (mapIn->keySize_bytes+mapIn->valueSize_bytes);

	// set some reasonable defaults
	mapIn->insertIndex = 0;
}


bool cxa_map_put(cxa_map_t *const mapIn, void *keyIn, void *valueIn)
{
	cxa_assert(mapIn);
	cxa_assert(keyIn);
	cxa_assert(valueIn);

	// make sure we have enough space in the array
	if( mapIn->insertIndex >= mapIn->maxNumElements ) return false;
	
	// if we made it here, we have enough elements, get ready to copy the item
	memcpy((void*)(mapIn->bufferLoc + (mapIn->insertIndex * (mapIn->keySize_bytes + mapIn->valueSize_bytes))), keyIn, mapIn->keySize_bytes);
	memcpy((void*)(mapIn->bufferLoc + (mapIn->insertIndex * (mapIn->keySize_bytes + mapIn->valueSize_bytes)) + mapIn->keySize_bytes), valueIn, mapIn->valueSize_bytes);
	mapIn->insertIndex++;

	// if we made it here, everything was successful
	return true;
}


void* cxa_map_put_empty(cxa_map_t *const mapIn, void *keyIn)
{
	cxa_assert(mapIn);
	cxa_assert(keyIn);
	
	// make sure we have enough space in the array
	if( mapIn->insertIndex >= mapIn->maxNumElements ) return NULL;
	
	memcpy((void*)(mapIn->bufferLoc + (mapIn->insertIndex * (mapIn->keySize_bytes + mapIn->valueSize_bytes))), keyIn, mapIn->keySize_bytes);
	void *retVal = (void*)(mapIn->bufferLoc + (mapIn->insertIndex * (mapIn->keySize_bytes + mapIn->valueSize_bytes)) + mapIn->keySize_bytes);
	mapIn->insertIndex++;
	
	return retVal;
}


bool cxa_map_removeElement(cxa_map_t *const mapIn, void *keyIn)
{
	cxa_assert(mapIn);
	cxa_assert(keyIn);
	
	size_t index = 0;
	if( !getIndex_byKey(mapIn, keyIn, &index) ) return false;
	
	// if this is the last element in the array, we don't need to do any memmoves
	if( (index+1) >= mapIn->insertIndex )
	{
		mapIn->insertIndex--;
		return true;
	}
	
	// if we made it here, we have some data to move around
	void *dest = (void*)(mapIn->bufferLoc + (index * (mapIn->keySize_bytes + mapIn->valueSize_bytes)));
	void *src = (void*)(mapIn->bufferLoc + (index+1 * (mapIn->keySize_bytes + mapIn->valueSize_bytes)));
	
	memmove(dest, src, ((mapIn->insertIndex-(index+1)) * (mapIn->keySize_bytes + mapIn->valueSize_bytes)));
	mapIn->insertIndex--;
	
	return true;
}


void* cxa_map_get(cxa_map_t *const mapIn, void *keyIn)
{
	cxa_assert(mapIn);

	size_t index = 0;
	if( !getIndex_byKey(mapIn, keyIn, &index) ) return NULL;
	return (void*)(mapIn->bufferLoc + (index * (mapIn->keySize_bytes + mapIn->valueSize_bytes)) + mapIn->keySize_bytes);
}


const size_t cxa_map_getSize_elems(cxa_map_t *const mapIn)
{
	cxa_assert(mapIn);

	return mapIn->insertIndex;
}


const size_t cxa_map_getMaxSize_elems(cxa_map_t *const mapIn)
{
	cxa_assert(mapIn);
	
	return mapIn->maxNumElements;
}


const bool cxa_map_isFull(cxa_map_t *const mapIn)
{
	cxa_assert(mapIn);
	
	return (mapIn->insertIndex >= mapIn->maxNumElements);
}


const bool cxa_map_isEmpty(cxa_map_t *const mapIn)
{
	cxa_assert(mapIn);
	
	return (mapIn->insertIndex == 0);
}


void cxa_map_clear(cxa_map_t *const mapIn)
{
	cxa_assert(mapIn);
	
	mapIn->insertIndex = 0;
}


size_t cxa_map_getFreeSize_elems(cxa_map_t *const mapIn)
{
	cxa_assert(mapIn);
	
	return (mapIn->maxNumElements - mapIn->insertIndex);
}


// ******** local function implementations ********
static bool getIndex_byKey(cxa_map_t *const mapIn, void *keyIn, size_t *indexOut)
{
	cxa_assert(mapIn);
	cxa_assert(keyIn);
	cxa_assert(indexOut);
	
	// iterate through our elements
	for( size_t i = 0; i < mapIn->insertIndex; i++ )
	{
		void *currKeyLoc = (void*)(mapIn->bufferLoc + (i * (mapIn->keySize_bytes + mapIn->valueSize_bytes)));
		
		if( memcmp(currKeyLoc, keyIn, mapIn->keySize_bytes) == 0 )
		{
			*indexOut = i;
			return true;
		}		
	}
	
	// didn't find the key...
	return false;
}
