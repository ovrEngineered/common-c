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
#include "cxa_led.h"


// ******** includes ********
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_led_init(cxa_led_t *const ledIn,
				  cxa_led_scm_setSolid_t scm_setSolidIn,
				  cxa_led_scm_blink_t scm_blinkIn,
				  cxa_led_scm_flashOnce_t scm_flashOnceIn)
{
	cxa_assert(ledIn);
	cxa_assert(scm_setSolidIn);

	// save our references
	ledIn->scm_setSolid = scm_setSolidIn;
	ledIn->scm_blink = scm_blinkIn;
	ledIn->scm_flashOnce = scm_flashOnceIn;

	// make sure we're initially off
	cxa_led_setSolid(ledIn, 0);
}


void cxa_led_blink(cxa_led_t *const ledIn, uint8_t onBrightness_255In, uint32_t onPeriod_msIn, uint32_t offPeriod_msIn)
{
	cxa_assert(ledIn);
	cxa_assert(ledIn->scm_blink);

	ledIn->prevState = ledIn->currState;
	ledIn->currState = CXA_LED_STATE_BLINK;
	ledIn->scm_blink(ledIn, onBrightness_255In, onPeriod_msIn, offPeriod_msIn);
}


void cxa_led_flashOnce(cxa_led_t *const ledIn, uint8_t brightness_255In, uint32_t period_msIn)
{
	cxa_assert(ledIn);
	cxa_assert(ledIn->scm_flashOnce);
	if( ledIn->currState == CXA_LED_STATE_FLASH_ONCE) return;

	ledIn->prevState = ledIn->currState;
	ledIn->currState = CXA_LED_STATE_FLASH_ONCE;
	ledIn->scm_flashOnce(ledIn, brightness_255In, period_msIn);
}


void cxa_led_setSolid(cxa_led_t *const ledIn, uint8_t brightness_255In)
{
	cxa_assert(ledIn);

	ledIn->prevState = ledIn->currState;
	ledIn->currState = CXA_LED_STATE_SOLID;
	ledIn->scm_setSolid(ledIn, brightness_255In);
}


// ******** local function implementations ********
