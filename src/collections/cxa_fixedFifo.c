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
}


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
	memcpy((void*)(fifoIn->bufferLoc + (fifoIn->insertIndex * fifoIn->datatypeSize_bytes)), elemIn, fifoIn->datatypeSize_bytes);
	fifoIn->insertIndex++;
	if( fifoIn->insertIndex >= fifoIn->maxNumElements ) fifoIn->insertIndex = 0;
	
	return true;
}


bool cxa_fixedFifo_dequeue(cxa_fixedFifo_t *const fifoIn, void *elemOut)
{
	cxa_assert(fifoIn);
	
	// if we're empty, we have nothing to return
	if( cxa_fixedFifo_isEmpty(fifoIn) )
	{
		return false;
	}
	
	// if we made it here, we should return our element
	if( elemOut != NULL )
	{
		memcpy(elemOut, (const void*)(fifoIn->bufferLoc + (fifoIn->removeIndex * fifoIn->datatypeSize_bytes)), fifoIn->datatypeSize_bytes);
	}
	fifoIn->removeIndex++;
	if( fifoIn->removeIndex >= fifoIn->maxNumElements ) fifoIn->removeIndex = 0;
	
	return true;
}


size_t cxa_fixedFifo_getCurrSize(cxa_fixedFifo_t *const fifoIn)
{
	cxa_assert(fifoIn);
	
	return (fifoIn->insertIndex >= fifoIn->removeIndex) ?
		(fifoIn->insertIndex - fifoIn->removeIndex) :
		((fifoIn->maxNumElements-fifoIn->removeIndex) + fifoIn->insertIndex);
}


bool cxa_fixedFifo_isFull(cxa_fixedFifo_t *const fifoIn)
{
	cxa_assert(fifoIn);
	
	return (fifoIn->removeIndex != 0) ?
		((fifoIn->removeIndex-1) == fifoIn->insertIndex) :
		(fifoIn->insertIndex == (fifoIn->maxNumElements-1));
}


bool cxa_fixedFifo_isEmpty(cxa_fixedFifo_t *const fifoIn)
{
	cxa_assert(fifoIn);
	
	return (fifoIn->insertIndex == fifoIn->removeIndex);
}


// ******** local function implementations ********

