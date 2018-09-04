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
#ifndef CXA_RUNLOOP_INFOPRINT_PERIOD_MS
	#define CXA_RUNLOOP_INFOPRINT_PERIOD_MS				10000
#endif

#define CXA_RUNLOOP_INFOPRINT_AVGNUMITERS				100


// ******** local type definitions ********
typedef struct
{
	int threadId;
	cxa_runLoop_cb_update_t cb;
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
void cxa_runLoop_addEntry(int threadIdIn, cxa_runLoop_cb_update_t cbIn, void *const userVarIn)
{
	if( !isInit ) cxa_runLoop_init();

	// create our new entry
	cxa_runLoop_entry_t newEntry = {.threadId = threadIdIn, .cb=cbIn, .userVar=userVarIn};
	cxa_assert_msg(cxa_array_append(&cbs, &newEntry), "increase CXA_RUNLOOP_MAXNUM_ENTRIES");
}


void cxa_runLoop_removeEntry(int threadIdIn, cxa_runLoop_cb_update_t cbIn)
{
	if( !isInit ) cxa_runLoop_init();

	// can't use cxa_array_iterate because we need an index
	for( size_t i = 0; i < cxa_array_getSize_elems(&cbs); i++ )
	{
		cxa_runLoop_entry_t* currEntry = (cxa_runLoop_entry_t*)cxa_array_get(&cbs, i);
		if( currEntry == NULL ) continue;

		if( (currEntry->threadId == threadIdIn) && (currEntry->cb == cbIn) )
		{
			cxa_array_remove_atIndex(&cbs, i);
			return;
		}
	}
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

	cxa_array_iterate(&cbs, currEntry, cxa_runLoop_entry_t)
	{
		if( (currEntry->threadId == threadIdIn) && (currEntry->cb != NULL) ) currEntry->cb(currEntry->userVar);
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

	// setup our info printing stuff
	uint32_t averageIterPeriod_us = 0;
	cxa_timeDiff_t td_printInfo;
	cxa_timeDiff_init(&td_printInfo);

	// start the iterations
	while(1)
	{
		uint32_t lastIterPeriod_ms = cxa_runLoop_iterate(threadIdIn);

#if CXA_RUNLOOP_INFOPRINT_PERIOD_MS > 0
		averageIterPeriod_us -= averageIterPeriod_us / CXA_RUNLOOP_INFOPRINT_AVGNUMITERS;
		averageIterPeriod_us += lastIterPeriod_ms;

		if( cxa_timeDiff_isElapsed_recurring_ms(&td_printInfo, CXA_RUNLOOP_INFOPRINT_PERIOD_MS) )
		{
			cxa_logger_debug(&logger, "threadId: %d  avgIterPeriod: %d ms", threadIdIn, averageIterPeriod_us / 1000);
		}
#endif
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

