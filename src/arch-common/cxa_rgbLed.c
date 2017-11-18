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
#include "cxa_rgbLed.h"


// ******** includes ********
#include <cxa_assert.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_rgbLed_init(cxa_rgbLed_t *const ledIn,
					 cxa_rgbLed_scm_setRgb_t scm_setRgbIn,
					 cxa_rgbLed_scm_blink_t scm_blinkIn,
					 cxa_rgbLed_scm_alternateColors_t scm_alternateColorsIn,
					 cxa_rgbLed_scm_flashOnce_t scm_flashOnceIn)
{
	cxa_assert(ledIn);

	// save our references
	ledIn->scm_setRgb = scm_setRgbIn;
	ledIn->scm_blink = scm_blinkIn;
	ledIn->scm_alternateColors = scm_alternateColorsIn;
	ledIn->scm_flashOnce = scm_flashOnceIn;

	ledIn->currState = CXA_RGBLED_STATE_SOLID;
}


void cxa_rgbLed_setRgb(cxa_rgbLed_t *const ledIn, uint8_t rIn, uint8_t gIn, uint8_t bIn)
{
	cxa_assert(ledIn);
	cxa_assert(ledIn->scm_setRgb);
	ledIn->prevState = ledIn->currState;
	ledIn->currState = CXA_RGBLED_STATE_SOLID;
	ledIn->scm_setRgb(ledIn, rIn, gIn, bIn);
}


void cxa_rgbLed_blink(cxa_rgbLed_t *const ledIn, uint8_t rIn, uint8_t gIn, uint8_t bIn,
					  uint16_t onPeriod_msIn, uint16_t offPeriod_msIn)
{
	cxa_assert(ledIn);
	cxa_assert(ledIn->scm_blink);
	ledIn->prevState = ledIn->currState;
	ledIn->currState = CXA_RGBLED_STATE_BLINK;
	ledIn->scm_blink(ledIn, rIn, gIn, bIn, onPeriod_msIn, offPeriod_msIn);
}


void cxa_rgbLed_alternateColors(cxa_rgbLed_t *const ledIn,
							   uint8_t r1In, uint8_t g1In, uint8_t b1In,
							   uint16_t color1Period_msIn,
							   uint8_t r2In, uint8_t g2In, uint8_t b2In,
							   uint16_t color2Period_msIn)
{
	cxa_assert(ledIn);
	cxa_assert(ledIn->scm_alternateColors);
	ledIn->prevState = ledIn->currState;
	ledIn->currState = CXA_RGBLED_STATE_ALTERNATE_COLORS;
	ledIn->scm_alternateColors(ledIn, r1In, g1In, b1In, color1Period_msIn, r2In, g2In, b2In, color2Period_msIn);
}


void cxa_rgbLed_flashOnce(cxa_rgbLed_t *const ledIn, uint8_t rIn, uint8_t gIn, uint8_t bIn,
						  uint16_t period_msIn)
{
	cxa_assert(ledIn);
	cxa_assert(ledIn->scm_flashOnce);
	if( ledIn->currState == CXA_RGBLED_STATE_FLASHONCE) return;

	ledIn->prevState = ledIn->currState;
	ledIn->currState = CXA_RGBLED_STATE_FLASHONCE;
	ledIn->scm_flashOnce(ledIn, rIn, gIn, bIn, period_msIn);
}

// ******** local function implementations ********
