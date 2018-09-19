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
#include "cxa_led_runLoop.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_runLoop.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_setSolid(cxa_led_t *const superIn, uint8_t brightness_255In);
static void scm_blink(cxa_led_t *const superIn, uint8_t onBrightness_255In, uint32_t onPeriod_msIn, uint32_t offPeriod_msIn);
static void scm_flashOnce(cxa_led_t *const superIn, uint8_t brightness_255In, uint32_t period_msIn);

static void cb_onRunLoopUpdate(void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_led_runLoop_init(cxa_led_runLoop_t *const ledIn,
						  cxa_led_runLoop_scm_setBrightness_t scm_setBrightnessIn,
						  int threadIdIn)
{
	cxa_assert(ledIn);
	cxa_assert(scm_setBrightnessIn);

	// save our references
	ledIn->scms.setBrightness = scm_setBrightnessIn;

	// setup our internal state
	cxa_timeDiff_init(&ledIn->td_gp);

	// initialize our super class (since it sets our initial value)
	cxa_led_init(&ledIn->super, scm_setSolid, scm_blink, scm_flashOnce);

	// register for run loop execution
	cxa_runLoop_addEntry(threadIdIn, NULL, cb_onRunLoopUpdate, (void*)ledIn);
}


// ******** local function implementations ********
static void scm_setSolid(cxa_led_t *const superIn, uint8_t brightness_255In)
{
	cxa_led_runLoop_t* ledIn = (cxa_led_runLoop_t*)superIn;
	cxa_assert(ledIn);

	ledIn->solid.lastBrightness_255 = brightness_255In;

	ledIn->scms.setBrightness(ledIn, brightness_255In);
}


static void scm_blink(cxa_led_t *const superIn, uint8_t onBrightness_255In, uint32_t onPeriod_msIn, uint32_t offPeriod_msIn)
{
	cxa_led_runLoop_t* ledIn = (cxa_led_runLoop_t*)superIn;
	cxa_assert(ledIn);

	ledIn->blink.onBrightness_255 = onBrightness_255In;
	ledIn->blink.wasOn = true;
	ledIn->blink.onPeriod_ms = onPeriod_msIn;
	ledIn->blink.offPeriod_ms = offPeriod_msIn;

	ledIn->scms.setBrightness(ledIn, onBrightness_255In);
}


static void scm_flashOnce(cxa_led_t *const superIn, uint8_t brightness_255In, uint32_t period_msIn)
{
	cxa_led_runLoop_t* ledIn = (cxa_led_runLoop_t*)superIn;
	cxa_assert(ledIn);

	ledIn->flash.period_ms = period_msIn;
	cxa_timeDiff_setStartTime_now(&ledIn->td_gp);

	ledIn->scms.setBrightness(ledIn, brightness_255In);
}


static void cb_onRunLoopUpdate(void* userVarIn)
{
	cxa_led_runLoop_t*ledIn = (cxa_led_runLoop_t*)userVarIn;
	cxa_assert(ledIn);

	switch( ledIn->super.currState )
	{
		case CXA_LED_STATE_BLINK:
			if( cxa_timeDiff_isElapsed_recurring_ms(&ledIn->td_gp, (cxa_gpio_getValue(ledIn->gpio) ? ledIn->blink.onPeriod_ms : ledIn->blink.offPeriod_ms)) )
			{
				if( cxa_gpio_getValue(ledIn->gpio) )
				{
					ledIn->blink.wasOn = false;
					ledIn->scms.setBrightness(ledIn, 0);
				}
				else
				{
					ledIn->blink.wasOn = true;
					ledIn->scms.setBrightness(ledIn, ledIn->blink.onBrightness_255);
				}
			}
			break;

		case CXA_LED_STATE_FLASH_ONCE:
			if( cxa_timeDiff_isElapsed_recurring_ms(&ledIn->td_gp, ledIn->flash.period_ms) )
			{
				if( ledIn->super.prevState == CXA_LED_STATE_BLINK )
				{
					ledIn->scms.setBrightness(ledIn, ledIn->blink.wasOn ? ledIn->blink.onBrightness_255 : 0);
					cxa_timeDiff_setStartTime_now(&ledIn->td_gp);
				}
				else if( ledIn->super.prevState == CXA_LED_STATE_SOLID )
				{
					ledIn->scms.setBrightness(ledIn, ledIn->solid.lastBrightness_255);
				}
			}
			break;

		case CXA_LED_STATE_SOLID:
			break;
	}
}
