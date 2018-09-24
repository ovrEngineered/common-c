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
#include "cxa_rgbLed_runLoop.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_runLoop.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_setRgb(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn);
static void scm_alternateColors(cxa_rgbLed_t *const superIn,
								uint8_t r1In, uint8_t g1In, uint8_t b1In,
								uint16_t color1Period_msIn,
								uint8_t r2In, uint8_t g2In, uint8_t b2In,
								uint16_t color2Period_msIn);
static void scm_flashOnce(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn,
	   	   	   	   	   	  uint16_t period_msIn);

static void cb_onRunLoopUpdate(void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_rgbLed_runLoop_init(cxa_rgbLed_runLoop_t *const ledIn,
							 cxa_rgbLed_runLoop_scm_setRgb_t scm_setRgbIn,
							 int threadIdIn)
{
	cxa_assert(ledIn);
	cxa_assert(scm_setRgbIn);

	// save our references
	ledIn->scms.setRgb = scm_setRgbIn;

	// setup our internal state
	cxa_timeDiff_init(&ledIn->td_gp);

	// initialize our super class
	cxa_rgbLed_init(&ledIn->super, scm_setRgb, scm_alternateColors, scm_flashOnce);

	// register for run loop execution
	cxa_runLoop_addEntry(threadIdIn, NULL, cb_onRunLoopUpdate, (void*)ledIn);
}


// ******** local function implementations ********
static void scm_setRgb(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn)
{
	cxa_rgbLed_runLoop_t* ledIn = (cxa_rgbLed_runLoop_t*)superIn;
	cxa_assert(ledIn);

	ledIn->solid.lastBrightnessR_255 = rIn;
	ledIn->solid.lastBrightnessG_255 = gIn;
	ledIn->solid.lastBrightnessB_255 = bIn;

	// set our individual brightnesses
	ledIn->scms.setRgb(ledIn, ledIn->solid.lastBrightnessR_255, ledIn->solid.lastBrightnessG_255, ledIn->solid.lastBrightnessB_255);
}


static void scm_alternateColors(cxa_rgbLed_t *const superIn,
								uint8_t r1In, uint8_t g1In, uint8_t b1In,
								uint16_t color1Period_msIn,
								uint8_t r2In, uint8_t g2In, uint8_t b2In,
								uint16_t color2Period_msIn)
{
	cxa_rgbLed_runLoop_t* ledIn = (cxa_rgbLed_runLoop_t*)superIn;
	cxa_assert(ledIn);

	ledIn->alternate.colors[0].rOn_255 = r1In;
	ledIn->alternate.colors[0].gOn_255 = g1In;
	ledIn->alternate.colors[0].bOn_255 = b1In;
	ledIn->alternate.colors[0].period_ms = color1Period_msIn;

	ledIn->alternate.colors[1].rOn_255 = r2In;
	ledIn->alternate.colors[1].gOn_255 = g2In;
	ledIn->alternate.colors[1].bOn_255 = b2In;
	ledIn->alternate.colors[1].period_ms = color2Period_msIn;

	ledIn->alternate.lastColorIndex = 0;

	// set our individual brightnesses
	ledIn->scms.setRgb(ledIn, r1In, g1In, b1In);
}


static void scm_flashOnce(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn,
	   	   	   	   	   	  uint16_t period_msIn)
{
	cxa_rgbLed_runLoop_t* ledIn = (cxa_rgbLed_runLoop_t*)superIn;
	cxa_assert(ledIn);

	ledIn->flash.period_ms = period_msIn;
	cxa_timeDiff_setStartTime_now(&ledIn->td_gp);

	// set our individual brightnesses
	ledIn->scms.setRgb(ledIn, rIn, gIn, bIn);
}


static void cb_onRunLoopUpdate(void* userVarIn)
{
	cxa_rgbLed_runLoop_t *ledIn = (cxa_rgbLed_runLoop_t*)userVarIn;
	cxa_assert(ledIn);


	switch( ledIn->super.currState )
	{
		case CXA_RGBLED_STATE_ALTERNATE_COLORS:
			if( cxa_timeDiff_isElapsed_recurring_ms(&ledIn->td_gp, (ledIn->alternate.lastColorIndex == 0) ? ledIn->alternate.colors[0].period_ms : ledIn->alternate.colors[1].period_ms) )
			{
				ledIn->alternate.lastColorIndex = (ledIn->alternate.lastColorIndex > 0) ? 0 : 1;

				// set our individual brightnesses
				ledIn->scms.setRgb(ledIn,
								   ledIn->alternate.colors[ledIn->alternate.lastColorIndex].rOn_255,
								   ledIn->alternate.colors[ledIn->alternate.lastColorIndex].gOn_255,
								   ledIn->alternate.colors[ledIn->alternate.lastColorIndex].bOn_255);
			}
			break;

		case CXA_RGBLED_STATE_FLASHONCE:
			if( cxa_timeDiff_isElapsed_recurring_ms(&ledIn->td_gp, ledIn->flash.period_ms) )
			{
				if( ledIn->super.prevState == CXA_RGBLED_STATE_ALTERNATE_COLORS )
				{
					// call the superclass to reset the internal state
					cxa_rgbLed_alternateColors(&ledIn->super,
							ledIn->alternate.colors[ledIn->alternate.lastColorIndex].rOn_255,
							ledIn->alternate.colors[ledIn->alternate.lastColorIndex].gOn_255,
							ledIn->alternate.colors[ledIn->alternate.lastColorIndex].bOn_255,
							ledIn->alternate.colors[ledIn->alternate.lastColorIndex].period_ms,
							ledIn->alternate.colors[!ledIn->alternate.lastColorIndex].rOn_255,
							ledIn->alternate.colors[!ledIn->alternate.lastColorIndex].gOn_255,
							ledIn->alternate.colors[!ledIn->alternate.lastColorIndex].bOn_255,
							ledIn->alternate.colors[!ledIn->alternate.lastColorIndex].period_ms);
				}
				else if( ledIn->super.prevState == CXA_RGBLED_STATE_SOLID )
				{
					// call the superclass to reset the internal state
					cxa_rgbLed_setRgb(&ledIn->super, ledIn->solid.lastBrightnessR_255, ledIn->solid.lastBrightnessG_255, ledIn->solid.lastBrightnessB_255);
				}
			}
			break;

		case CXA_RGBLED_STATE_SOLID:
			break;
	}
}
