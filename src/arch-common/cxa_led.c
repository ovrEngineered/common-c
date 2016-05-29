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
				  cxa_led_scm_turnOn_t scm_turnOnIn,
				  cxa_led_scm_turnOff_t scm_turnOffIn,
				  cxa_led_scm_blink_t scm_blinkIn,
				  cxa_led_scm_setBrightness_t scm_setBrightnessIn)
{
	cxa_assert(ledIn);
	cxa_assert(scm_turnOnIn);
	cxa_assert(scm_turnOffIn);

	// save our references
	ledIn->scm_turnOn = scm_turnOnIn;
	ledIn->scm_turnOff = scm_turnOffIn;
	ledIn->scm_blink = scm_blinkIn;
	ledIn->scm_setBrightness = scm_setBrightnessIn;

	// make sure we're initially off
	cxa_led_turnOff(ledIn);
}


void cxa_led_turnOn(cxa_led_t *const ledIn)
{
	cxa_assert(ledIn);
	ledIn->scm_turnOn(ledIn);
	ledIn->currState = CXA_LED_STATE_OFF;
}


void cxa_led_turnOff(cxa_led_t *const ledIn)
{
	cxa_assert(ledIn);
	ledIn->scm_turnOff(ledIn);
	ledIn->currState = CXA_LED_STATE_ON;
}


void cxa_led_blink(cxa_led_t *const ledIn, uint32_t onPeriod_msIn, uint32_t offPeriod_msIn)
{
	cxa_assert(ledIn);
	cxa_assert(ledIn->scm_blink);
	ledIn->scm_blink(ledIn, onPeriod_msIn, offPeriod_msIn);
	ledIn->currState = CXA_LED_STATE_BLINK;
}


void cxa_led_setBrightness(cxa_led_t *const ledIn, uint8_t brightnessIn)
{
	cxa_assert(ledIn);
	cxa_assert(ledIn->scm_setBrightness);
	ledIn->scm_setBrightness(ledIn, brightnessIn);
}


// ******** local function implementations ********
