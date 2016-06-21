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
#include "cxa_rgbLed_pca9624.h"


// ******** includes ********
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_setRgb(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_rgbLed_pca9624_init(cxa_rgbLed_pca9624_t *const ledIn, cxa_pca9624_t *const pcaIn,
							uint8_t chanIndex_rIn, uint8_t chanIndex_gIn, uint8_t chanIndex_bIn)
{
	cxa_assert(ledIn);
	cxa_assert(chanIndex_rIn <= CXA_PCA9624_NUM_CHANNELS);
	cxa_assert(chanIndex_gIn <= CXA_PCA9624_NUM_CHANNELS);
	cxa_assert(chanIndex_bIn <= CXA_PCA9624_NUM_CHANNELS);

	// save our references
	ledIn->pca = pcaIn;
	ledIn->chanIndex_r = chanIndex_rIn;
	ledIn->chanIndex_g = chanIndex_gIn;
	ledIn->chanIndex_b = chanIndex_bIn;

	// initialize our super class
	cxa_rgbLed_init(&ledIn->super, scm_setRgb);
}


// ******** local function implementations ********
static void scm_setRgb(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn)
{
	cxa_rgbLed_pca9624_t* ledIn = (cxa_rgbLed_pca9624_t*)superIn;
	cxa_assert(ledIn);

	// set our brightnesses
	cxa_pca9624_channelEntry_t chanEntries[] = {
						{.channelIndex=ledIn->chanIndex_r, .brightness=rIn},
						{.channelIndex=ledIn->chanIndex_g, .brightness=gIn},
						{.channelIndex=ledIn->chanIndex_b, .brightness=bIn}
				};
	cxa_pca9624_setBrightnessForChannels(ledIn->pca, chanEntries, (sizeof(chanEntries)/sizeof(*chanEntries)));
}
