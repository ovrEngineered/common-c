/**
 * Copyright 2015 opencxa.org
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
#include "cxa_gpio_debouncer.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_runLoop.h>


// ******** local macro definitions ********
#define TIMEOUT_PERIOD_MS				100


// ******** local type definitions ********


// ******** local function prototypes ********
static void cb_onRunLoopUpdate(void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_gpio_debouncer_init(cxa_gpio_debouncer_t *const debounceIn, cxa_gpio_t *const gpioIn)
{
	cxa_assert(debounceIn);
	cxa_assert(gpioIn);
	cxa_assert(cxa_gpio_getDirection(gpioIn) == CXA_GPIO_DIR_INPUT);

	// save our references
	debounceIn->gpio = gpioIn;

	// set some initial values
	debounceIn->prevVal = cxa_gpio_getValue(debounceIn->gpio);
	debounceIn->isInTimeoutPeriod = false;

	// setup our timediff
	cxa_timeDiff_init(&debounceIn->td_debounce, false);

	// setup our listener array
	cxa_array_initStd(&debounceIn->listeners, debounceIn->listeners_raw);

	// register for run loop execution
	cxa_runLoop_addEntry(cb_onRunLoopUpdate, (void*)debounceIn);
}


void cxa_gpio_debouncer_addListener(cxa_gpio_debouncer_t *const debouncerIn, cxa_gpio_debouncer_cb_onTransition_t cb_onTransition, void *userVarIn)
{
	cxa_assert(debouncerIn);

	cxa_gpio_debouncer_listenerEntry_t newEntry = {
			.cb_onTransition = cb_onTransition,
			.userVar = userVarIn
	};
	cxa_assert( cxa_array_append(&debouncerIn->listeners, &newEntry) );
}


bool cxa_gpio_debouncer_getCurrentValue(cxa_gpio_debouncer_t *const debouncerIn)
{
	cxa_assert(debouncerIn);

	return cxa_gpio_getValue(debouncerIn->gpio);
}


// ******** local function implementations ********
static void cb_onRunLoopUpdate(void* userVarIn)
{
	cxa_gpio_debouncer_t* debouncerIn = (cxa_gpio_debouncer_t*)userVarIn;
	cxa_assert(debouncerIn);

	if( debouncerIn->isInTimeoutPeriod )
	{
		if( cxa_timeDiff_isElapsed_ms(&debouncerIn->td_debounce, TIMEOUT_PERIOD_MS) )
		{
			// recheck our value
			bool currVal = cxa_gpio_getValue(debouncerIn->gpio);
			if( currVal != debouncerIn->prevVal )
			{
				// got a transition...notify our listeners
				cxa_array_iterate(&debouncerIn->listeners, currListener, cxa_gpio_debouncer_listenerEntry_t)
				{
					if( currListener == NULL ) continue;
					if( currListener->cb_onTransition != NULL ) currListener->cb_onTransition(currVal, currListener->userVar);
				}
			}

			// reset
			debouncerIn->prevVal = currVal;
			debouncerIn->isInTimeoutPeriod = false;
		}
	}
	else
	{
		// not in our timeout...look for new values
		bool currVal = cxa_gpio_getValue(debouncerIn->gpio);
		if( currVal != debouncerIn->prevVal ) debouncerIn->isInTimeoutPeriod = true;
	}
}
