/**
 * Copyright 2013-2015 opencxa.org
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
#include "cxa_ble112_timeBase.h"

#include <blestack/hw.h>

#include <cxa_assert.h>


// ******** local macro definitions ********
#define TICKS_TO_US(ticksIn)				((125 * (ticksIn)) / 4)			// hw_timer_ticks() / 32000 * 1E6


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_ble112_timeBase_init(cxa_timeBase_t *const tbIn)
{
	cxa_assert(tbIn);
	// nothing to do here
}


uint32_t cxa_timeBase_getCount_us(cxa_timeBase_t *const superIn)
{
	cxa_assert(superIn);
	
	return TICKS_TO_US(hw_timer_ticks());
}


uint32_t cxa_timeBase_getMaxCount_us(cxa_timeBase_t *const superIn)
{
	cxa_assert(superIn);
	
	// 2^24 - 1
	return TICKS_TO_US(16777215);
}


// ******** local function implementations ********

