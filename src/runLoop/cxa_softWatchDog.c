/**
 * @copyright 2017 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Christopher Armenio
 */
#include "cxa_softWatchDog.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_runLoop.h>

#define CXA_LOG_LEVEL				CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void cb_onRunLoopUpdate(void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_softWatchDog_init(cxa_softWatchDog_t *const swdIn, uint32_t timeoutPeriod_msIn, int threadIdIn,
						   cxa_softWatchDog_cb_t cbIn, void *const userVarIn)
{
	cxa_assert(swdIn);
	cxa_assert(cbIn);

	// save our references
	swdIn->isPaused = true;
	swdIn->cb = cbIn;
	swdIn->userVar = userVarIn;
	swdIn->timeoutPeriod_ms = timeoutPeriod_msIn;
	cxa_timeDiff_init(&swdIn->td_timeout);

	// register for our runloop
	cxa_runLoop_addEntry(threadIdIn, NULL, cb_onRunLoopUpdate, (void*)swdIn);
}


void cxa_softWatchDog_kick(cxa_softWatchDog_t *const swdIn)
{
	cxa_assert(swdIn);

	cxa_timeDiff_setStartTime_now(&swdIn->td_timeout);
	swdIn->isPaused = false;
}


void cxa_softWatchDog_pause(cxa_softWatchDog_t *const swdIn)
{
	cxa_assert(swdIn);

	swdIn->isPaused = true;
}


bool cxa_softWatchDog_isPaused(cxa_softWatchDog_t *const swdIn)
{
	cxa_assert(swdIn);

	return swdIn->isPaused;
}


// ******** local function implementations ********
static void cb_onRunLoopUpdate(void* userVarIn)
{
	cxa_softWatchDog_t* swdIn = (cxa_softWatchDog_t*)userVarIn;
	cxa_assert(swdIn);

	if( !swdIn->isPaused && cxa_timeDiff_isElapsed_ms(&swdIn->td_timeout, swdIn->timeoutPeriod_ms) )
	{
		// watchdog expired...call our callback once and only once
		if( swdIn->cb != NULL) swdIn->cb(swdIn->userVar);
		swdIn->isPaused = true;
	}
}
