/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_oneShotTimer.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_runLoop.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void cb_onRunLoopUpdate(void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_oneShotTimer_init(cxa_oneShotTimer_t *const ostIn, int threadIdIn)
{
	cxa_assert(ostIn);

	// set our initial state
	ostIn->isActive = false;
	cxa_timeDiff_init(&ostIn->timeDiff);

	// register for runLoop updates
	cxa_runLoop_addEntry(threadIdIn, NULL, cb_onRunLoopUpdate, (void*)ostIn);
}


void cxa_oneShotTimer_schedule(cxa_oneShotTimer_t *const ostIn, uint32_t delay_msIn, cxa_oneShotTimer_cb_t cbIn, void *const userVarIn)
{
	cxa_assert(ostIn);

	ostIn->isActive = false;

	ostIn->delay_ms = delay_msIn;
	ostIn->cb = cbIn;
	ostIn->userVar = userVarIn;
	cxa_timeDiff_setStartTime_now(&ostIn->timeDiff);

	ostIn->isActive = true;
}


// ******** local function implementations ********
static void cb_onRunLoopUpdate(void* userVarIn)
{
	cxa_oneShotTimer_t* ostIn = (cxa_oneShotTimer_t*)userVarIn;
	cxa_assert(ostIn);

	if( ostIn->isActive && cxa_timeDiff_isElapsed_ms(&ostIn->timeDiff, ostIn->delay_ms) )
	{
		ostIn->isActive = false;
		if( ostIn->cb != NULL ) ostIn->cb(ostIn->userVar);
	}
}
