/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_runLoop.h"


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
typedef enum
{
	STATE_UNUSED,
	STATE_RESERVED_CONFIGURING,
	STATE_RESERVED_CONFIGURED_UNSTARTED,
	STATE_RESERVED_CONFIGURED_STARTED
}state_t;


typedef enum
{
	TYPE_STANDARD,
	TYPE_ONESHOT
}type_t;


typedef struct
{
	state_t state;
	type_t type;

	int threadId;

	uint32_t execPeriod_ms;
	cxa_timeDiff_t td_exec;

	cxa_runLoop_cb_t startupCb;
	cxa_runLoop_cb_t updateCb;
	void *userVar;
}cxa_runLoop_entry_t;


// ******** local function prototypes ********
static void init(void);
static cxa_runLoop_entry_t* reserveUnusedEntry(void);


// ********  local variable declarations *********
static bool isInit = false;

static cxa_runLoop_entry_t entries[CXA_RUNLOOP_MAXNUM_ENTRIES];

static cxa_logger_t logger;


// ******** global function implementations ********
void cxa_runLoop_addEntry(int threadIdIn, cxa_runLoop_cb_t startupCbIn, cxa_runLoop_cb_t updateCbIn, void *const userVarIn)
{
	if( !isInit ) init();

	cxa_runLoop_entry_t* newEntry = reserveUnusedEntry();
	cxa_assert_msg(newEntry, "increase CXA_RUNLOOP_MAXNUM_ENTRIES");

	newEntry->threadId = threadIdIn;
	newEntry->type = TYPE_STANDARD;
	newEntry->execPeriod_ms=0;
	newEntry->startupCb=startupCbIn;
	newEntry->updateCb=updateCbIn;
	newEntry->userVar=userVarIn;
	cxa_timeDiff_init(&newEntry->td_exec);
	newEntry->state = STATE_RESERVED_CONFIGURED_UNSTARTED;
}


void cxa_runLoop_addTimedEntry(int threadIdIn, uint32_t execPeriod_msIn, cxa_runLoop_cb_t startupCbIn, cxa_runLoop_cb_t updateCbIn, void *const userVarIn)
{
	if( !isInit ) init();

	cxa_runLoop_entry_t* newEntry = reserveUnusedEntry();
	cxa_assert_msg(newEntry, "increase CXA_RUNLOOP_MAXNUM_ENTRIES");

	newEntry->threadId = threadIdIn;
	newEntry->type = TYPE_STANDARD;
	newEntry->execPeriod_ms=execPeriod_msIn;
	newEntry->startupCb=startupCbIn;
	newEntry->updateCb=updateCbIn;
	newEntry->userVar=userVarIn;
	cxa_timeDiff_init(&newEntry->td_exec);
	newEntry->state = STATE_RESERVED_CONFIGURED_UNSTARTED;
}


void cxa_runLoop_clearAllEntries(void)
{
	isInit = false;
	init();
}


void cxa_runLoop_dispatchNextIteration(int threadIdIn, cxa_runLoop_cb_t updateCbIn, void *const userVarIn)
{
	if( !isInit ) init();

	cxa_runLoop_entry_t* newEntry = reserveUnusedEntry();
	cxa_assert_msg(newEntry, "increase CXA_RUNLOOP_MAXNUM_ENTRIES");

	newEntry->threadId = threadIdIn;
	newEntry->type = TYPE_ONESHOT;
	newEntry->execPeriod_ms=0;
	newEntry->startupCb=NULL;
	newEntry->updateCb=updateCbIn;
	newEntry->userVar=userVarIn;
	cxa_timeDiff_init(&newEntry->td_exec);
	newEntry->state = STATE_RESERVED_CONFIGURED_UNSTARTED;
}


void cxa_runLoop_dispatchAfter(int threadIdIn, uint32_t delay_msIn, cxa_runLoop_cb_t updateCbIn, void *const userVarIn)
{
	if( !isInit ) init();

	cxa_runLoop_entry_t* newEntry = reserveUnusedEntry();
	cxa_assert_msg(newEntry, "increase CXA_RUNLOOP_MAXNUM_ENTRIES");

	newEntry->threadId = threadIdIn;
	newEntry->type = TYPE_ONESHOT;
	newEntry->execPeriod_ms=delay_msIn;
	newEntry->startupCb=NULL;
	newEntry->updateCb=updateCbIn;
	newEntry->userVar=userVarIn;
	cxa_timeDiff_init(&newEntry->td_exec);
	newEntry->state = STATE_RESERVED_CONFIGURED_UNSTARTED;
}


uint32_t cxa_runLoop_iterate(int threadIdIn)
{
	if( !isInit ) init();

	uint32_t iter_startTime_us = cxa_timeBase_getCount_us();

	// iterate first and make sure all of our entries have been started
	for( size_t i = 0; i < sizeof(entries)/sizeof(*entries); i++ )
	{
		if( (entries[i].threadId == threadIdIn) &&
			(entries[i].state == STATE_RESERVED_CONFIGURED_UNSTARTED) )
		{
			if( entries[i].startupCb != NULL ) entries[i].startupCb(entries[i].userVar);
			entries[i].state = STATE_RESERVED_CONFIGURED_STARTED;
		}
	}

	// now iterate again and call our update functions
	for( size_t i = 0; i < sizeof(entries)/sizeof(*entries); i++ )
	{
		if( (entries[i].threadId == threadIdIn) &&
			(entries[i].state == STATE_RESERVED_CONFIGURED_STARTED) )
		{
			// we know this is valid callback for this thread...
			// if it's timed, make sure we're calling it at the right pace
			if( (entries[i].execPeriod_ms == 0) ||
				 cxa_timeDiff_isElapsed_recurring_ms(&entries[i].td_exec, entries[i].execPeriod_ms) )
			{
				if( entries[i].updateCb != NULL ) entries[i].updateCb(entries[i].userVar);

				// free this entry if it's a one-shot
				if( entries[i].type == TYPE_ONESHOT ) entries[i].state = STATE_UNUSED;
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
	if( !isInit ) init();

	// start the iterations
	while(1)
	{
		cxa_runLoop_iterate(threadIdIn);
	}
}


// ******** local function implementations ********
static void init(void)
{
	if( isInit ) return;

	for( size_t i = 0; i < sizeof(entries)/sizeof(*entries); i++ )
	{
		entries[i].state = STATE_UNUSED;
	}
	cxa_logger_init(&logger, "runLoop");

	isInit = true;
}


static cxa_runLoop_entry_t* reserveUnusedEntry(void)
{
	for( size_t i = 0; i < sizeof(entries)/sizeof(*entries); i++ )
	{
		if( entries[i].state == STATE_UNUSED )
		{
			entries[i].state = STATE_RESERVED_CONFIGURING;
			return &entries[i];
		}
	}

	return NULL;
}
