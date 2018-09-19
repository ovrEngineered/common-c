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
#include "cxa_atmega_led_pwm8.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_runLoop.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_setBrightness(cxa_led_runLoop_t *const superIn, uint8_t brightness_255In);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_atmega_led_pwm8_init(cxa_atmega_led_pwm8_t *const ledIn, cxa_atmega_timer8_ocr_t *const ocrIn, int threadIdIn)
{
	cxa_assert(ledIn);
	cxa_assert(ocrIn);

	// save our references
	ledIn->ocr = ocrIn;

	// initialize our super class (since it sets our initial value)
	cxa_led_runLoop_init(&ledIn->super, scm_setBrightness, threadIdIn);
}


// ******** local function implementations ********
static void scm_setBrightness(cxa_led_runLoop_t *const superIn, uint8_t brightness_255In)
{
	cxa_atmega_led_pwm8_t* ledIn = (cxa_atmega_led_pwm8_t*)superIn;
	cxa_assert(ledIn);

	cxa_atmega_timer8_ocr_setValue(ledIn->ocr, brightness_255In);
}
