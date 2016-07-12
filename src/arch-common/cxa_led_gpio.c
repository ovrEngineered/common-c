/**
 * @copyright 2015 opencxa.org
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
#include "cxa_led_gpio.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_runLoop.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_turnOn(cxa_led_t *const superIn);
static void scm_turnOff(cxa_led_t *const superIn);
static void scm_blink(cxa_led_t *const superIn, uint32_t onPeriod_msIn, uint32_t offPeriod_msIn);

static void cb_onRunLoopUpdate(void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_led_gpio_init(cxa_led_gpio_t *const ledIn, cxa_gpio_t *const gpioIn, bool driveOffStateIn)
{
	cxa_assert(ledIn);
	cxa_assert(gpioIn);

	// save our references
	ledIn->gpio = gpioIn;
	ledIn->driveOffState = driveOffStateIn;

	// setup our internal state
	cxa_timeDiff_init(&ledIn->td_blink, true);

	// initialize our super class (since it sets our initial value)
	cxa_led_init(&ledIn->super, scm_turnOn, scm_turnOff, scm_blink, NULL);

	// register for run loop execution
	cxa_runLoop_addEntry(cb_onRunLoopUpdate, (void*)ledIn);
}


// ******** local function implementations ********
static void scm_turnOn(cxa_led_t *const superIn)
{
	cxa_led_gpio_t* ledIn = (cxa_led_gpio_t*)superIn;
	cxa_assert(ledIn);

	if( !ledIn->driveOffState ) cxa_gpio_setDirection(ledIn->gpio, CXA_GPIO_DIR_OUTPUT);
	cxa_gpio_setValue(ledIn->gpio, true);
}


static void scm_turnOff(cxa_led_t *const superIn)
{
	cxa_led_gpio_t* ledIn = (cxa_led_gpio_t*)superIn;
	cxa_assert(ledIn);

	if( ledIn->driveOffState ) cxa_gpio_setValue(ledIn->gpio, false);
	else cxa_gpio_setDirection(ledIn->gpio, CXA_GPIO_DIR_INPUT);
}


static void scm_blink(cxa_led_t *const superIn, uint32_t onPeriod_msIn, uint32_t offPeriod_msIn)
{
	cxa_led_gpio_t* ledIn = (cxa_led_gpio_t*)superIn;
	cxa_assert(ledIn);

	ledIn->onPeriod_ms = onPeriod_msIn;
	ledIn->offPeriod_ms = offPeriod_msIn;
}


static void cb_onRunLoopUpdate(void* userVarIn)
{
	cxa_led_gpio_t*ledIn = (cxa_led_gpio_t*)userVarIn;
	cxa_assert(ledIn);

	if( (ledIn->super.currState == CXA_LED_STATE_BLINK) &&
			cxa_timeDiff_isElapsed_recurring_ms(&ledIn->td_blink, (cxa_gpio_getValue(ledIn->gpio) ? ledIn->onPeriod_ms : ledIn->offPeriod_ms)) )
	{
		if( cxa_gpio_getValue(ledIn->gpio) ) scm_turnOff(&ledIn->super);
		else scm_turnOn(&ledIn->super);
	}
}
