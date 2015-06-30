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
#ifndef CXA_TIMEDIFF_H_
#define CXA_TIMEDIFF_H_


/**
 * @file
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdint.h>
#include <stdbool.h>
#include <cxa_timeBase.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct 
{
	cxa_timeBase_t *tb;
	
	bool isFirstCycle;
	uint32_t startTime_us;
}cxa_timeDiff_t;


// ******** global function prototypes ********
void cxa_timeDiff_init(cxa_timeDiff_t *const tdIn, cxa_timeBase_t *const tbIn, bool setStartTimeIn);

void cxa_timeDiff_setStartTime_now(cxa_timeDiff_t *const tdIn);

uint32_t cxa_timeDiff_getElapsedTime_ms(cxa_timeDiff_t *const tdIn);

bool cxa_timeDiff_isElapsed_ms(cxa_timeDiff_t *const tdIn, uint32_t msIn);

bool cxa_timeDiff_isElaped_recurring_ms(cxa_timeDiff_t *const tdIn, uint32_t msIn);


#endif // CXA_timeBase_H_
