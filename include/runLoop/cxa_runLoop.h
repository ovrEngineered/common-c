/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_RUN_LOOP_H_
#define CXA_RUN_LOOP_H_


// ******** includes ********
#include <stdbool.h>
#include <stdint.h>
#include <cxa_config.h>


// ******** global macro definitions ********
#ifndef CXA_RUNLOOP_MAXNUM_ENTRIES
	#define CXA_RUNLOOP_MAXNUM_ENTRIES				10
#endif

#define CXA_RUNLOOP_THREADID_DEFAULT				0


// ******** global type definitions *********
/**
 * @public
 */
typedef void (*cxa_runLoop_cb_t)(void* userVarIn);


// ******** global function prototypes ********
void cxa_runLoop_addEntry(int threadIdIn, cxa_runLoop_cb_t startupCbIn, cxa_runLoop_cb_t updateCbIn, void *const userVarIn);
void cxa_runLoop_addTimedEntry(int threadIdIn, uint32_t execPeriod_msIn, cxa_runLoop_cb_t startupCbIn, cxa_runLoop_cb_t updateCbIn, void *const userVarIn);
void cxa_runLoop_clearAllEntries(void);

void cxa_runLoop_dispatchNextIteration(int threadIdIn, cxa_runLoop_cb_t updateCbIn, void *const userVarIn);
void cxa_runLoop_dispatchAfter(int threadIdIn, uint32_t delay_msIn, cxa_runLoop_cb_t updateCbIn, void *const userVarIn);

uint32_t cxa_runLoop_iterate(int threadIdIn);
void cxa_runLoop_execute(int threadIdIn);


#endif // CXA_RUN_LOOP_H_
