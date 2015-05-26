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
#include "cxa_timeDiff.h"


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
void cxa_timeDiff_init(cxa_timeDiff_t *const tdIn, cxa_timeBase_t *const tbIn)
{
	cxa_assert(tdIn);
	cxa_assert(tbIn);
	
	// save our references
	tdIn->tb = tbIn;
	
	// set some appropriate initial values
	tdIn->startTime_us = 0;
}


void cxa_timeDiff_setStartTime_now(cxa_timeDiff_t *const tdIn)
{
	cxa_assert(tdIn);
	
	tdIn->startTime_us = cxa_timeBase_getCount_us(tdIn->tb);	
}


uint32_t cxa_timeDiff_getElapsedTime_ms(cxa_timeDiff_t *const tdIn)
{
	cxa_assert(tdIn);
	
	uint32_t curr_us = cxa_timeBase_getCount_us(tdIn->tb);
	uint32_t elapsedTime_us = (curr_us >= tdIn->startTime_us) ?
							  (curr_us - tdIn->startTime_us) :
							  ((cxa_timeBase_getMaxCount_us(tdIn->tb) - tdIn->startTime_us) + curr_us);
	return elapsedTime_us / 1000;
}


bool cxa_timeDiff_isElapsed_ms(cxa_timeDiff_t *const tdIn, uint32_t msIn)
{
	cxa_assert(tdIn);
	
	return (cxa_timeDiff_getElapsedTime_ms(tdIn) >= msIn);
}


bool cxa_timeDiff_isElaped_recurring_ms(cxa_timeDiff_t *const tdIn, uint32_t msIn)
{
	cxa_assert(tdIn);
	
	bool retVal = (cxa_timeDiff_getElapsedTime_ms(tdIn) >= msIn);
	if( retVal ) cxa_timeDiff_setStartTime_now(tdIn);
	
	return retVal;
}


// ******** local function implementations ********

