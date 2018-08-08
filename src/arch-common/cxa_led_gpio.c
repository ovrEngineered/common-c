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
static void scm_setBrightness(cxa_led_t *const superIn, uint8_t brightnessIn);
static void scm_flashOnce(cxa_led_t *const superIn, bool flashStateIn, uint32_t period_msIn);

static void cb_onRunLoopUpdate(void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_led_gpio_init(cxa_led_gpio_t *const ledIn, cxa_gpio_t *const gpioIn, int threadIdIn)
{
	cxa_assert(ledIn);
	cxa_assert(gpioIn);

	// save our references
	ledIn->gpio = gpioIn;

	// setup our internal state
	cxa_timeDiff_init(&ledIn->td_gp);

	// initialize our super class (since it sets our initial value)
	cxa_led_init(&ledIn->super, scm_turnOn, scm_turnOff, scm_blink, scm_setBrightness, scm_flashOnce);

	// register for run loop execution
	cxa_runLoop_addEntry(threadIdIn, cb_onRunLoopUpdate, (void*)ledIn);
}


// ******** local function implementations ********
static void scm_turnOn(cxa_led_t *const superIn)
{
	cxa_led_gpio_t* ledIn = (cxa_led_gpio_t*)superIn;
	cxa_assert(ledIn);

	cxa_gpio_setValue(ledIn->gpio, true);
}


static void scm_turnOff(cxa_led_t *const superIn)
{
	cxa_led_gpio_t* ledIn = (cxa_led_gpio_t*)superIn;
	cxa_assert(ledIn);

	cxa_gpio_setValue(ledIn->gpio, false);
}


static void scm_blink(cxa_led_t *const superIn, uint32_t onPeriod_msIn, uint32_t offPeriod_msIn)
{
	cxa_led_gpio_t* ledIn = (cxa_led_gpio_t*)superIn;
	cxa_assert(ledIn);

	ledIn->blink.onPeriod_ms = onPeriod_msIn;
	ledIn->blink.offPeriod_ms = offPeriod_msIn;
}


static void scm_setBrightness(cxa_led_t *const superIn, uint8_t brightnessIn)
{
	cxa_led_gpio_t* ledIn = (cxa_led_gpio_t*)superIn;
	cxa_assert(ledIn);

	if( brightnessIn > 0 ) scm_turnOn(superIn);
	else scm_turnOff(superIn);
}


static void scm_flashOnce(cxa_led_t *const superIn, bool flashStateIn, uint32_t period_msIn)
{
	cxa_led_gpio_t* ledIn = (cxa_led_gpio_t*)superIn;
	cxa_assert(ledIn);

	// turn on or off (don't call super class since it'll change our 'state')
	if( flashStateIn ) scm_turnOn(superIn);
	else scm_turnOff(superIn);

	ledIn->flash.period_ms = period_msIn;
	cxa_timeDiff_setStartTime_now(&ledIn->td_gp);
}


static void cb_onRunLoopUpdate(void* userVarIn)
{
	cxa_led_gpio_t*ledIn = (cxa_led_gpio_t*)userVarIn;
	cxa_assert(ledIn);

	switch( ledIn->super.currState )
	{
		case CXA_LED_STATE_BLINK:
			if( cxa_timeDiff_isElapsed_recurring_ms(&ledIn->td_gp, (cxa_gpio_getValue(ledIn->gpio) ? ledIn->blink.onPeriod_ms : ledIn->blink.offPeriod_ms)) )
			{
				if( cxa_gpio_getValue(ledIn->gpio) ) scm_turnOff(&ledIn->super);
				else scm_turnOn(&ledIn->super);
			}
			break;

		case CXA_LED_STATE_FLASH_ONCE:
			if( cxa_timeDiff_isElapsed_recurring_ms(&ledIn->td_gp, ledIn->flash.period_ms) )
			{
				if( ledIn->super.prevState == CXA_LED_STATE_BLINK )
				{
					cxa_led_blink(&ledIn->super, ledIn->blink.onPeriod_ms, ledIn->blink.offPeriod_ms);
				}
				else if( ledIn->super.prevState == CXA_LED_STATE_ON )
				{
					cxa_led_turnOn(&ledIn->super);
				}
				else if( ledIn->super.prevState == CXA_LED_STATE_OFF )
				{
					cxa_led_turnOff(&ledIn->super);
				}
			}
			break;

		case CXA_LED_STATE_ON:
			break;

		case CXA_LED_STATE_OFF:
			break;
	}
}
