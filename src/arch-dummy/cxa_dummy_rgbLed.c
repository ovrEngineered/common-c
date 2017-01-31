/**
 * @copyright 2017 opencxa.org
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
#include "cxa_dummy_rgbLed.h"


// ******** includes ********
#include <cxa_assert.h>


#define CXA_LOG_LEVEL					CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_setRgb(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_dummy_rgbLed_init(cxa_dummy_rgbLed_t *const ledIn, const char *const nameIn)
{
	cxa_assert(ledIn);
	cxa_assert(nameIn);

	// save our references / internal state
	cxa_logger_init_formattedString(&ledIn->logger, "rgbLed-%s", nameIn);

	// initialize our super class
	cxa_rgbLed_init(&ledIn->super, scm_setRgb, NULL);
}


// ******** local function implementations ********
static void scm_setRgb(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn)
{
	cxa_dummy_rgbLed_t* ledIn = (cxa_dummy_rgbLed_t*)superIn;
	cxa_assert(ledIn);

	cxa_logger_trace(&ledIn->logger, "r: %d  g: %d  b: %d", rIn, gIn, bIn);
}
