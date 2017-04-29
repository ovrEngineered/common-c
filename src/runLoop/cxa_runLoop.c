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

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>
#include <cxa_config.h>

// ******** local macro definitions ********
#ifndef CXA_RUNLOOP_MAXNUM_THREADS
	#define CXA_RUNLOOP_MAXNUM_THREADS					3
#endif

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
static cxa_timeDiff_t td_printInfo;

#if CXA_RUNLOOP_INFOPRINT_PERIOD_MS > 0
static uint32_t averageIterPeriod_us = 0;
#endif


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



#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
void cxa_runLoop_iterate(int threadIdIn)
{
	if( !isInit ) cxa_runLoop_init();

#if CXA_RUNLOOP_INFOPRINT_PERIOD_MS > 0
	uint32_t iter_startTime_us = cxa_timeBase_getCount_us();
#endif

	cxa_array_iterate(&cbs, currEntry, cxa_runLoop_entry_t)
	{
		if( (currEntry->threadId == threadIdIn) && (currEntry->cb != NULL) ) currEntry->cb(currEntry->userVar);
	}

#if CXA_RUNLOOP_INFOPRINT_PERIOD_MS > 0
	uint32_t iter_time_us = cxa_timeBase_getCount_us() - iter_startTime_us;
	averageIterPeriod_us -= averageIterPeriod_us / CXA_RUNLOOP_INFOPRINT_AVGNUMITERS;
	averageIterPeriod_us += iter_time_us;

	if( cxa_timeDiff_isElapsed_recurring_ms(&td_printInfo, CXA_RUNLOOP_INFOPRINT_PERIOD_MS) )
	{
		cxa_logger_debug(&logger, "iteration  curr: %d ms  avg: %d ms", iter_time_us / 1000, averageIterPeriod_us / 1000);
	}
#endif

	taskYIELD();
}


void cxa_runLoop_execute(int threadIdIn)
{
	if( !isInit ) cxa_runLoop_init();

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
	cxa_timeDiff_init(&td_printInfo);

	isInit = true;
}

