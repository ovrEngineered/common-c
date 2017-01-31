/**
 * @copyright 2017 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Christopher Armenio
 */
#include "cxa_dummy_led.h"


// ******** includes ********
#include <cxa_assert.h>

#define CXA_LOG_LEVEL				CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_turnOn(cxa_led_t *const superIn);
static void scm_turnOff(cxa_led_t *const superIn);
static void scm_blink(cxa_led_t *const superIn, uint32_t onPeriod_msIn, uint32_t offPeriod_msIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_dummy_led_init(cxa_dummy_led_t *const ledIn, const char *const nameIn)
{
	cxa_assert(ledIn);
	cxa_assert(nameIn);

	// save our references / setup our internal state
	cxa_logger_init_formattedString(&ledIn->logger, "led-%s", nameIn);

	// initialize our super class (since it sets our initial value)
	cxa_led_init(&ledIn->super, scm_turnOn, scm_turnOff, scm_blink, NULL);
}


// ******** local function implementations ********
static void scm_turnOn(cxa_led_t *const superIn)
{
	cxa_dummy_led_t* ledIn = (cxa_dummy_led_t*)superIn;
	cxa_assert(ledIn);

	cxa_logger_trace(&ledIn->logger, "turn on");
}


static void scm_turnOff(cxa_led_t *const superIn)
{
	cxa_dummy_led_t* ledIn = (cxa_dummy_led_t*)superIn;
	cxa_assert(ledIn);

	cxa_logger_trace(&ledIn->logger, "turn off");
}


static void scm_blink(cxa_led_t *const superIn, uint32_t onPeriod_msIn, uint32_t offPeriod_msIn)
{
	cxa_dummy_led_t* ledIn = (cxa_dummy_led_t*)superIn;
	cxa_assert(ledIn);

	cxa_logger_trace(&ledIn->logger, "blink %dms / %dms", onPeriod_msIn, offPeriod_msIn);
}
