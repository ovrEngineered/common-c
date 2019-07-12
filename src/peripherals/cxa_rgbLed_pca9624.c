/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_rgbLed_pca9624.h"


// ******** includes ********
#include <cxa_assert.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_setRgb(cxa_rgbLed_runLoop_t *const superIn, uint8_t r_255In, uint8_t g_255In, uint8_t b_255In);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_rgbLed_pca9624_init(cxa_rgbLed_pca9624_t *const ledIn, int threadIdIn,
							cxa_pca9624_t *const pcaIn, uint8_t maxBrightnessIn,
							uint8_t chanIndex_rIn, uint8_t chanIndex_gIn, uint8_t chanIndex_bIn)
{
	cxa_assert(ledIn);
	cxa_assert(chanIndex_rIn <= CXA_PCA9624_NUM_CHANNELS);
	cxa_assert(chanIndex_gIn <= CXA_PCA9624_NUM_CHANNELS);
	cxa_assert(chanIndex_bIn <= CXA_PCA9624_NUM_CHANNELS);

	// save our references
	ledIn->pca = pcaIn;
	ledIn->maxBrightness = maxBrightnessIn;
	ledIn->chanIndex_r = chanIndex_rIn;
	ledIn->chanIndex_g = chanIndex_gIn;
	ledIn->chanIndex_b = chanIndex_bIn;

	// initialize our super class
	cxa_rgbLed_runLoop_init(&ledIn->super, scm_setRgb, threadIdIn);
}


// ******** local function implementations ********
static void scm_setRgb(cxa_rgbLed_runLoop_t *const superIn, uint8_t r_255In, uint8_t g_255In, uint8_t b_255In)
{
	cxa_rgbLed_pca9624_t* ledIn = (cxa_rgbLed_pca9624_t*)superIn;
	cxa_assert(ledIn);

	// adjust for our max brightness
	r_255In = (((float)r_255In) / 255.0) * ledIn->maxBrightness;
	g_255In = (((float)g_255In) / 255.0) * ledIn->maxBrightness;
	b_255In = (((float)b_255In) / 255.0) * ledIn->maxBrightness;

	// set our brightnesses
	cxa_pca9624_channelEntry_t chanEntries[] = {
						{.channelIndex=ledIn->chanIndex_r, .brightness=r_255In},
						{.channelIndex=ledIn->chanIndex_g, .brightness=g_255In},
						{.channelIndex=ledIn->chanIndex_b, .brightness=b_255In}
				};
	cxa_pca9624_setBrightnessForChannels(ledIn->pca, chanEntries, (sizeof(chanEntries)/sizeof(*chanEntries)));
}
