/**
 * Copyright 2018 opencxa.org
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
 *
 * @author Christopher Armenio
 */
#include <cxa_atmega_timeBase.h>

#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void timer8_cb_onOverflow(cxa_atmega_timer8_t *const timerIn, void *userVarIn);


// ********  local variable declarations *********
static cxa_atmega_timer8_t* timer = NULL;
static uint32_t numOverflows = 0;


// ******** global function implementations ********
void cxa_atmega_timeBase_initWithTimer8(cxa_atmega_timer8_t *const timerIn)
{
	cxa_assert(timerIn);

	timer = timerIn;
	cxa_atmega_timer8_addListener(timer, timer8_cb_onOverflow, NULL);
	cxa_atmega_timer8_enableInterrupt_overflow(timer);
}


uint32_t cxa_timeBase_getCount_us(void)
{
	return (timer != NULL) ? (numOverflows * cxa_atmega_timer8_getOverflowPeriod_us(timer)) : 0;
}


uint32_t cxa_timeBase_getMaxCount_us(void)
{
	return UINT32_MAX;
}


// ******** local function implementations ********
static void timer8_cb_onOverflow(cxa_atmega_timer8_t *const timerIn, void *userVarIn)
{
	numOverflows++;
}
