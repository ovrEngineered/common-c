/**
 * @copyright 2016 opencxa.org
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
#include "cxa_rgbLed_triLed.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_runLoop.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_setRgb(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn);
static void scm_blink(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn,
		   	   	   	  uint16_t onPeriod_msIn, uint16_t offPeriod_msIn);
static void scm_flashOnce(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn,
	   	   	   	   	   	  uint16_t period_msIn);

static void cb_onRunLoopUpdate(void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_rgbLed_triLed_init(cxa_rgbLed_triLed_t *const ledIn,
							cxa_led_t *const led_rIn, cxa_led_t *const led_gIn, cxa_led_t *const led_bIn,
							int threadIdIn)
{
	cxa_assert(ledIn);
	cxa_assert(led_rIn);
	cxa_assert(led_gIn);
	cxa_assert(led_bIn);

	// save our references
	ledIn->led_r = led_rIn;
	ledIn->led_g = led_gIn;
	ledIn->led_b = led_bIn;

	// setup our internal state
	cxa_timeDiff_init(&ledIn->td_gp);
	ledIn->blink.r = 0;
	ledIn->blink.g = 0;
	ledIn->blink.b = 0;
	ledIn->blink.isOn = false;

	// initialize our super class
	cxa_rgbLed_init(&ledIn->super, scm_setRgb, scm_blink, NULL, scm_flashOnce);

	// register for run loop execution
	cxa_runLoop_addEntry(threadIdIn, cb_onRunLoopUpdate, (void*)ledIn);
}


// ******** local function implementations ********
static void scm_setRgb(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn)
{
	cxa_rgbLed_triLed_t* ledIn = (cxa_rgbLed_triLed_t*)superIn;
	cxa_assert(ledIn);

	ledIn->lastSet.r = rIn;
	ledIn->lastSet.g = gIn;
	ledIn->lastSet.b = bIn;

	// set our individual brightnesses
	cxa_led_setBrightness(ledIn->led_r, rIn);
	cxa_led_setBrightness(ledIn->led_g, gIn);
	cxa_led_setBrightness(ledIn->led_b, bIn);
}


static void scm_blink(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn,
		   	   	   	  uint16_t onPeriod_msIn, uint16_t offPeriod_msIn)
{
	cxa_rgbLed_triLed_t* ledIn = (cxa_rgbLed_triLed_t*)superIn;
	cxa_assert(ledIn);

	ledIn->blink.r = rIn;
	ledIn->blink.g = gIn;
	ledIn->blink.b = bIn;

	ledIn->blink.onPeriod_ms = onPeriod_msIn;
	ledIn->blink.offPeriod_ms = offPeriod_msIn;
}


static void scm_flashOnce(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn,
	   	   	   	   	   	  uint16_t period_msIn)
{
	cxa_rgbLed_triLed_t* ledIn = (cxa_rgbLed_triLed_t*)superIn;
	cxa_assert(ledIn);

	// set our individual brightnesses
	cxa_led_setBrightness(ledIn->led_r, rIn);
	cxa_led_setBrightness(ledIn->led_g, gIn);
	cxa_led_setBrightness(ledIn->led_b, bIn);

	ledIn->flash.period_ms = period_msIn;
	cxa_timeDiff_setStartTime_now(&ledIn->td_gp);
}


static void cb_onRunLoopUpdate(void* userVarIn)
{
	cxa_rgbLed_triLed_t *ledIn = (cxa_rgbLed_triLed_t*)userVarIn;
	cxa_assert(ledIn);


	switch( ledIn->super.currState )
	{
		case CXA_RGBLED_STATE_BLINK:
			if( cxa_timeDiff_isElapsed_recurring_ms(&ledIn->td_gp, (ledIn->blink.isOn) ? ledIn->blink.onPeriod_ms : ledIn->blink.offPeriod_ms) )
			{
				if( ledIn->blink.isOn )
				{
					// set our individual brightnesses
					cxa_led_setBrightness(ledIn->led_r, 0);
					cxa_led_setBrightness(ledIn->led_g, 0);
					cxa_led_setBrightness(ledIn->led_b, 0);
					ledIn->blink.isOn = false;
				}
				else
				{
					// set our individual brightnesses
					cxa_led_setBrightness(ledIn->led_r, ledIn->blink.r);
					cxa_led_setBrightness(ledIn->led_g, ledIn->blink.g);
					cxa_led_setBrightness(ledIn->led_b, ledIn->blink.b);
					ledIn->blink.isOn = true;
				}
			}
			break;

		case CXA_RGBLED_STATE_FLASHONCE:
			if( cxa_timeDiff_isElapsed_recurring_ms(&ledIn->td_gp, ledIn->flash.period_ms) )
			{
				if( ledIn->super.prevState == CXA_RGBLED_STATE_BLINK )
				{
					cxa_rgbLed_blink(&ledIn->super, ledIn->blink.r, ledIn->blink.g, ledIn->blink.b, ledIn->blink.onPeriod_ms, ledIn->blink.offPeriod_ms);
				}
				else if( ledIn->super.prevState == CXA_RGBLED_STATE_SOLID )
				{
					cxa_rgbLed_setRgb(&ledIn->super, ledIn->lastSet.r, ledIn->lastSet.g, ledIn->lastSet.b);
				}
			}
			break;

		case CXA_RGBLED_STATE_SOLID:
			break;

		case CXA_RGBLED_STATE_ALTERNATE_COLORS:
			break;
	}
}
