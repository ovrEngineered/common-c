/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_timeDiff.h"


// ******** includes ********
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_timeDiff_init(cxa_timeDiff_t *const tdIn)
{
	cxa_assert(tdIn);

	cxa_timeDiff_setStartTime_now(tdIn);
}


void cxa_timeDiff_setStartTime_now(cxa_timeDiff_t *const tdIn)
{
	cxa_assert(tdIn);

	tdIn->startTime_us = cxa_timeBase_getCount_us();
}


uint32_t cxa_timeDiff_getElapsedTime_ms(cxa_timeDiff_t *const tdIn)
{
	cxa_assert(tdIn);

	uint32_t curr_us = cxa_timeBase_getCount_us();
	uint32_t elapsedTime_us = (curr_us >= tdIn->startTime_us) ?
							  (curr_us - tdIn->startTime_us) :
							  ((cxa_timeBase_getMaxCount_us() - tdIn->startTime_us) + curr_us);
	return elapsedTime_us / 1000;
}


bool cxa_timeDiff_isElapsed_ms(cxa_timeDiff_t *const tdIn, uint32_t msIn)
{
	cxa_assert(tdIn);

	return (cxa_timeDiff_getElapsedTime_ms(tdIn) >= msIn);
}


bool cxa_timeDiff_isElapsed_recurring_ms(cxa_timeDiff_t *const tdIn, uint32_t msIn)
{
	cxa_assert(tdIn);

	bool retVal = cxa_timeDiff_isElapsed_ms(tdIn, msIn);
	if( retVal ) cxa_timeDiff_setStartTime_now(tdIn);

	return retVal;
}


// ******** local function implementations ********
