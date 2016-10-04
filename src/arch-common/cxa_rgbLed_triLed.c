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


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_setRgb(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_rgbLed_triLed_init(cxa_rgbLed_triLed_t *const ledIn,
							cxa_led_t *const led_rIn, cxa_led_t *const led_gIn, cxa_led_t *const led_bIn)
{
	cxa_assert(ledIn);
	cxa_assert(led_rIn);
	cxa_assert(led_gIn);
	cxa_assert(led_bIn);

	// save our references
	ledIn->led_r = led_rIn;
	ledIn->led_g = led_gIn;
	ledIn->led_b = led_bIn;

	// initialize our super class
	cxa_rgbLed_init(&ledIn->super, scm_setRgb, NULL);
}


// ******** local function implementations ********
static void scm_setRgb(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn)
{
	cxa_rgbLed_triLed_t* ledIn = (cxa_rgbLed_triLed_t*)superIn;
	cxa_assert(ledIn);

	// set our individual brightnesses
	cxa_led_setBrightness(ledIn->led_r, rIn);
	cxa_led_setBrightness(ledIn->led_g, gIn);
	cxa_led_setBrightness(ledIn->led_b, bIn);
}
