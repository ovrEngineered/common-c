/**
 * Copyright 2016 opencxa.org
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
#include "cxa_runLoop.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_timeDiff.h>

// include for our target build system
#ifdef __XC
    // microchip
    #include "system_definitions.h"
#elif defined ESP32
    // esp32
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include <esp_task_wdt.h>
#endif


#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>
#include <cxa_config.h>

// ******** local macro definitions ********


// ******** local type definitions ********
typedef struct
{
	int threadId;
	bool hasBeenStarted;

	uint32_t execPeriod_ms;
	cxa_timeDiff_t td_exec;

	cxa_runLoop_cb_t startupCb;
	cxa_runLoop_cb_t updateCb;
	void *userVar;
}cxa_runLoop_entry_t;


// ******** local function prototypes ********
static void cxa_runLoop_init(void);


// ********  local variable declarations *********
static bool isInit = false;

static cxa_array_t cbs;
static cxa_runLoop_entry_t cbs_raw[CXA_RUNLOOP_MAXNUM_ENTRIES];

static cxa_logger_t logger;


// ******** global function implementations ********
void cxa_runLoop_addEntry(int threadIdIn, cxa_runLoop_cb_t startupCbIn, cxa_runLoop_cb_t updateCbIn, void *const userVarIn)
{
	if( !isInit ) cxa_runLoop_init();

	// create our new entry
	cxa_runLoop_entry_t newEntry = {.threadId = threadIdIn, .hasBeenStarted=false, .execPeriod_ms=0, .startupCb=startupCbIn, .updateCb=updateCbIn, .userVar=userVarIn};
	cxa_assert_msg(cxa_array_append(&cbs, &newEntry), "increase CXA_RUNLOOP_MAXNUM_ENTRIES");
}


void cxa_runLoop_addTimedEntry(int threadIdIn, uint32_t execPeriod_msIn, cxa_runLoop_cb_t startupCbIn, cxa_runLoop_cb_t updateCbIn, void *const userVarIn)
{
	if( !isInit ) cxa_runLoop_init();

	// create our new entry
	cxa_runLoop_entry_t newEntry = {.threadId = threadIdIn, .hasBeenStarted=false, .execPeriod_ms=execPeriod_msIn, .startupCb=startupCbIn, .updateCb=updateCbIn, .userVar=userVarIn};
	cxa_timeDiff_init(&newEntry.td_exec);
	cxa_assert_msg(cxa_array_append(&cbs, &newEntry), "increase CXA_RUNLOOP_MAXNUM_ENTRIES");
}


void cxa_runLoop_clearAllEntries(void)
{
	isInit = false;
	cxa_runLoop_init();
}


uint32_t cxa_runLoop_iterate(int threadIdIn)
{
	if( !isInit ) cxa_runLoop_init();

	uint32_t iter_startTime_us = cxa_timeBase_getCount_us();

	// iterate first and make sure all of our entries have been started
	cxa_array_iterate(&cbs, currEntry, cxa_runLoop_entry_t)
	{
		if( (currEntry != NULL) && (currEntry->threadId == threadIdIn) &&
			!currEntry->hasBeenStarted && (currEntry->startupCb != NULL) )
		{
			currEntry->hasBeenStarted = true;
			currEntry->startupCb(currEntry->userVar);
		}
	}

	// now iterate again and call our update functions
	cxa_array_iterate(&cbs, currEntry, cxa_runLoop_entry_t)
	{
		if( (currEntry != NULL) && (currEntry->threadId == threadIdIn) &&
			(currEntry->updateCb != NULL) )
		{
			// we know this is valid callback for this thread...
			// if it's timed, make sure we're calling it at the right pace

			if( (currEntry->execPeriod_ms == 0) ||
				 cxa_timeDiff_isElapsed_recurring_ms(&currEntry->td_exec, currEntry->execPeriod_ms) )
			{
				currEntry->updateCb(currEntry->userVar);
			}
		}
	}

#ifdef ESP32
    esp_task_wdt_feed();        // esp32 only
#endif
    
#ifdef INC_FREERTOS_H
	taskYIELD();
#endif

	return cxa_timeBase_getCount_us() - iter_startTime_us;
}


void cxa_runLoop_execute(int threadIdIn)
{
	if( !isInit ) cxa_runLoop_init();

	// start the iterations
	while(1)
	{
		cxa_runLoop_iterate(threadIdIn);
	}
}


// ******** local function implementations ********
static void cxa_runLoop_init(void)
{
	if( isInit ) return;

	cxa_array_initStd(&cbs, cbs_raw);
	cxa_logger_init(&logger, "runLoop");

	isInit = true;
}

