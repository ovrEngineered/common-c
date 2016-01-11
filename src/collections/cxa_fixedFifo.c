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
#include "cxa_fixedFifo.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <string.h>
#include <stdint.h>
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_fixedFifo_init(cxa_fixedFifo_t *const fifoIn, cxa_fixedFifo_onFullAction_t onFullActionIn, const size_t datatypeSize_bytesIn, void *const bufferLocIn, const size_t bufferMaxSize_bytesIn)
{
	cxa_assert(fifoIn);
	cxa_assert( (onFullActionIn == CXA_FF_ON_FULL_DEQUEUE) ||
				(onFullActionIn == CXA_FF_ON_FULL_DROP) );
	cxa_assert(datatypeSize_bytesIn <= bufferMaxSize_bytesIn);
	cxa_assert(bufferLocIn);
	
	// save our references
	fifoIn->onFullAction = onFullActionIn;
	fifoIn->datatypeSize_bytes = datatypeSize_bytesIn;
	fifoIn->bufferLoc = bufferLocIn;
	fifoIn->maxNumElements = bufferMaxSize_bytesIn / datatypeSize_bytesIn;
	
	// set some reasonable defaults
	fifoIn->insertIndex = 0;
	fifoIn->removeIndex = 0;

	#if CXA_FF_MAX_LISTENERS > 0
		// setup our listener array
		cxa_array_initStd(&fifoIn->listeners, fifoIn->listeners_raw);
	#endif
}


#if CXA_FF_MAX_LISTENERS > 0
void cxa_fixedFifo_addListener(cxa_fixedFifo_t *const fifoIn, cxa_fixedFifo_cb_noLongerFull_t cb_noLongerFull, void* userVarIn)
{
	cxa_assert(fifoIn);

	cxa_fixedFifo_listener_entry_t newEntry = {.cb_noLongerFull=cb_noLongerFull, .userVarIn=userVarIn};
	cxa_assert(cxa_array_append(&fifoIn->listeners, &newEntry));
}
#endif


bool cxa_fixedFifo_queue(cxa_fixedFifo_t *const fifoIn, void *const elemIn)
{
	cxa_assert(fifoIn);
	cxa_assert(elemIn);
	
	// if we're full, figure out what we should do
	if( cxa_fixedFifo_isFull(fifoIn) )
	{
		switch( fifoIn->onFullAction )
		{
			case CXA_FF_ON_FULL_DEQUEUE:
				cxa_fixedFifo_dequeue(fifoIn, NULL);
				break;
			
			case CXA_FF_ON_FULL_DROP:
				return false;
		}
	}
	
	// if we made it here, we should add our element
	memcpy((void*)(((uint8_t*)fifoIn->bufferLoc) + (fifoIn->insertIndex * fifoIn->datatypeSize_bytes)), elemIn, fifoIn->datatypeSize_bytes);
	size_t newInsertIndex = fifoIn->insertIndex + 1;
	fifoIn->insertIndex = (newInsertIndex >= fifoIn->maxNumElements) ? 0 : newInsertIndex;
	
	return true;
}


bool cxa_fixedFifo_dequeue(cxa_fixedFifo_t *const fifoIn, void *elemOut)
{
	cxa_assert(fifoIn);
	
	#if CXA_FF_MAX_LISTENERS > 0
		bool wasFull = cxa_fixedFifo_isFull(fifoIn);
	#endif

	// if we're empty, we have nothing to return
	if( cxa_fixedFifo_isEmpty(fifoIn) )
	{
		return false;
	}
	
	// if we made it here, we should return our element
	if( elemOut != NULL )
	{
		memcpy(elemOut, (const void*)(((uint8_t*)fifoIn->bufferLoc) + (fifoIn->removeIndex * fifoIn->datatypeSize_bytes)), fifoIn->datatypeSize_bytes);
	}
	size_t newRemoveIndex = fifoIn->removeIndex + 1;
	fifoIn->removeIndex = (newRemoveIndex >= fifoIn->maxNumElements) ? 0 : newRemoveIndex;
	
	#if CXA_FF_MAX_LISTENERS > 0
		// notify our listeners
		if( wasFull )
		{
			cxa_array_iterate(&fifoIn->listeners, currEntry, cxa_fixedFifo_listener_entry_t)
			{
				if( currEntry == NULL ) continue;

				if( currEntry->cb_noLongerFull != NULL ) currEntry->cb_noLongerFull(fifoIn, currEntry->userVarIn);
			}
		}
	#endif

	return true;
}


bool cxa_fixedFifo_bulkQueue(cxa_fixedFifo_t *const fifoIn, void *const elemsIn, size_t numElemsIn)
{
	cxa_assert(fifoIn);
	cxa_assert(elemsIn);

	// simplistic implementation...we can probably make this more efficient
	for( size_t i = 0; i < numElemsIn; i++ )
	{
		if( !cxa_fixedFifo_queue(fifoIn, &(((uint8_t*)elemsIn)[i*fifoIn->datatypeSize_bytes])) ) return false;
	}

	return true;
}


bool cxa_fixedFifo_bulkDequeue(cxa_fixedFifo_t *const fifoIn, size_t numElemsIn)
{
	cxa_assert(fifoIn);

	// simplistic implementation...we can probably make this more efficient
	for( size_t i = 0; i < numElemsIn; i++ )
	{
		if( !cxa_fixedFifo_dequeue(fifoIn, NULL) ) return false;
	}

	return true;
}


size_t cxa_fixedFifo_bulkDequeue_peek(cxa_fixedFifo_t *const fifoIn, void **const elemsOut)
{
	cxa_assert(fifoIn);

	if( elemsOut != NULL ) *elemsOut = &(((uint8_t*)fifoIn->bufferLoc)[fifoIn->removeIndex*fifoIn->datatypeSize_bytes]);

	return (fifoIn->insertIndex >= fifoIn->removeIndex) ?
			(fifoIn->insertIndex - fifoIn->removeIndex) :
			(fifoIn->maxNumElements-fifoIn->removeIndex);
}


size_t cxa_fixedFifo_getSize_elems(cxa_fixedFifo_t *const fifoIn)
{
	cxa_assert(fifoIn);
	
	return (fifoIn->insertIndex >= fifoIn->removeIndex) ?
		(fifoIn->insertIndex - fifoIn->removeIndex) :
		((fifoIn->maxNumElements-fifoIn->removeIndex) + fifoIn->insertIndex);
}


size_t cxa_fixedFifo_getFreeSize_elems(cxa_fixedFifo_t *const fifoIn)
{
	cxa_assert(fifoIn);

	return fifoIn->maxNumElements - cxa_fixedFifo_getSize_elems(fifoIn);
}


bool cxa_fixedFifo_isFull(cxa_fixedFifo_t *const fifoIn)
{
	cxa_assert(fifoIn);
	
	size_t lcl_removeIndex = fifoIn->removeIndex;
	size_t lcl_insertIndex = fifoIn->insertIndex;

	return (lcl_removeIndex != 0) ?
		((lcl_removeIndex-1) == lcl_insertIndex) :
		(lcl_insertIndex == (fifoIn->maxNumElements-1));
}


bool cxa_fixedFifo_isEmpty(cxa_fixedFifo_t *const fifoIn)
{
	cxa_assert(fifoIn);
	
	return (fifoIn->insertIndex == fifoIn->removeIndex);
}


// ******** local function implementations ********

