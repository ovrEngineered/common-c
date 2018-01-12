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
#ifndef CXA_RUN_LOOP_H_
#define CXA_RUN_LOOP_H_


/**
 * @file
 *
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdbool.h>
#include <stdint.h>
#include <cxa_array.h>
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
typedef void (*cxa_runLoop_cb_update_t)(void* userVarIn);


// ******** global function prototypes ********
void cxa_runLoop_addEntry(int threadIdIn, cxa_runLoop_cb_update_t cbIn, void *const userVarIn);
void cxa_runLoop_removeEntry(int threadIdIn, cxa_runLoop_cb_update_t cbIn);
void cxa_runLoop_clearAllEntries(void);

uint32_t cxa_runLoop_iterate(int threadIdIn);
void cxa_runLoop_execute(int threadIdIn);


#endif // CXA_RUN_LOOP_H_
