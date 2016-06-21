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


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_rgbLed_init(cxa_rgbLed_t *const ledIn, cxa_rgbLed_scm_setRgb_t scm_setRgbIn)
{
	cxa_assert(ledIn);

	// save our references
	ledIn->scm_setRgb = scm_setRgbIn;
}


void cxa_rgbLed_setRgb(cxa_rgbLed_t *const ledIn, uint8_t rIn, uint8_t gIn, uint8_t bIn)
{
	cxa_assert(ledIn);
	cxa_assert(ledIn->scm_setRgb);
	ledIn->scm_setRgb(ledIn, rIn, gIn, bIn);
}


// ******** local function implementations ********
