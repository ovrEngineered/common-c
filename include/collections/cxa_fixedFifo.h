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
#ifndef CXA_FIXED_FIFO_H_
#define CXA_FIXED_FIFO_H_


/**
 * @file
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdlib.h>
#include <stdbool.h>
#include <cxa_array.h>


// ******** global macro definitions ********
#ifndef CXA_FF_MAX_LISTENERS
	#define CXA_FF_MAX_LISTENERS		1
#endif

#define cxa_fixedFifo_initStd(fifoIn, onFullActionIn, bufferIn)						cxa_fixedFifo_init((fifoIn), (onFullActionIn), sizeof(*(bufferIn)), ((void*)(bufferIn)), sizeof(bufferIn))


// ******** global type definitions *********
typedef struct cxa_fixedFifo cxa_fixedFifo_t;


typedef enum
{
	CXA_FF_ON_FULL_DEQUEUE,
	CXA_FF_ON_FULL_DROP
}cxa_fixedFifo_onFullAction_t;


typedef void (*cxa_fixedFifo_cb_noLongerFull_t)(cxa_fixedFifo_t *const fifoIn, void* userVarIn);


typedef struct
{
	cxa_fixedFifo_cb_noLongerFull_t cb_noLongerFull;

	void* userVarIn;
}cxa_fixedFifo_listener_entry_t;


struct cxa_fixedFifo
{
	void *bufferLoc;
	
	size_t insertIndex;
	size_t removeIndex;

	size_t datatypeSize_bytes;
	size_t maxNumElements;
	
	cxa_fixedFifo_onFullAction_t onFullAction;

	cxa_array_t listeners;
	cxa_fixedFifo_listener_entry_t listeners_raw[CXA_FF_MAX_LISTENERS];
};


// ******** global function prototypes ********
void cxa_fixedFifo_init(cxa_fixedFifo_t *const fifoIn, cxa_fixedFifo_onFullAction_t onFullActionIn, const size_t datatypeSize_bytesIn, void *const bufferLocIn, const size_t bufferMaxSize_bytesIn);

void cxa_fixedFifo_addListener(cxa_fixedFifo_t *const fifoIn, cxa_fixedFifo_cb_noLongerFull_t cb_noLongerFull, void* userVarIn);

bool cxa_fixedFifo_queue(cxa_fixedFifo_t *const fifoIn, void *const elemIn);

bool cxa_fixedFifo_dequeue(cxa_fixedFifo_t *const fifoIn, void *elemOut);

size_t cxa_fixedFifo_getCurrSize(cxa_fixedFifo_t *const fifoIn);
bool cxa_fixedFifo_isFull(cxa_fixedFifo_t *const fifoIn);
bool cxa_fixedFifo_isEmpty(cxa_fixedFifo_t *const fifoIn);


#endif // CXA_FIXED_FIFO_H_
