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

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_setRgb(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn);
static void scm_blink(cxa_rgbLed_t *constLedIn, uint8_t rIn, uint8_t gIn, uint8_t bIn,
					  uint16_t onPeriod_msIn, uint16_t offPeriod_msIn);
static void scm_alternateColors(cxa_rgbLed_t *const superIn,
							   uint8_t r1In, uint8_t g1In, uint8_t b1In,
							   uint16_t color1Period_msIn,
							   uint8_t r2In, uint8_t g2In, uint8_t b2In,
							   uint16_t color2Period_msIn);
static void scm_flashOnce(cxa_rgbLed_t *const superIn,
						  uint8_t rIn, uint8_t gIn, uint8_t bIn,
						  uint16_t period_msIn);

static void oneShotCb_blinkOn(void* userVarIn);
static void oneShotCb_blinkOff(void* userVarIn);

static void oneShotCb_alternate1(void* userVarIn);
static void oneShotCb_alternate2(void* userVarIn);

static void oneShotCb_flashOnce(void* userVarIn);


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
	ledIn->threadId = threadIdIn;
	ledIn->maxBrightness = maxBrightnessIn;
	ledIn->chanIndex_r = chanIndex_rIn;
	ledIn->chanIndex_g = chanIndex_gIn;
	ledIn->chanIndex_b = chanIndex_bIn;

	ledIn->currColor.r = 0;
	ledIn->currColor.g = 0;
	ledIn->currColor.b = 0;

	cxa_oneShotTimer_init(&ledIn->ost, threadIdIn);

	// initialize our super class
	cxa_rgbLed_init(&ledIn->super, scm_setRgb, scm_blink, scm_alternateColors, scm_flashOnce);
}


// ******** local function implementations ********
static void scm_setRgb(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn)
{
	cxa_rgbLed_pca9624_t* ledIn = (cxa_rgbLed_pca9624_t*)superIn;
	cxa_assert(ledIn);

	// save our current color
	ledIn->currColor.r = rIn;
	ledIn->currColor.g = gIn;
	ledIn->currColor.b = bIn;

	// adjust for our max brightness
	rIn = (((float)rIn) / 255.0) * ledIn->maxBrightness;
	gIn = (((float)gIn) / 255.0) * ledIn->maxBrightness;
	bIn = (((float)bIn) / 255.0) * ledIn->maxBrightness;

	// set our brightnesses
	cxa_pca9624_channelEntry_t chanEntries[] = {
						{.channelIndex=ledIn->chanIndex_r, .brightness=rIn},
						{.channelIndex=ledIn->chanIndex_g, .brightness=gIn},
						{.channelIndex=ledIn->chanIndex_b, .brightness=bIn}
				};
	cxa_pca9624_setBrightnessForChannels(ledIn->pca, chanEntries, (sizeof(chanEntries)/sizeof(*chanEntries)));
}


static void scm_blink(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn,
					  uint16_t onPeriod_msIn, uint16_t offPeriod_msIn)
{
	cxa_rgbLed_pca9624_t* ledIn = (cxa_rgbLed_pca9624_t*)superIn;
	cxa_assert(ledIn);

	// brightness adjustment is done lazily (before actually setting RGB)

	// save our settings
	ledIn->blink.onPeriod_msIn = onPeriod_msIn;
	ledIn->blink.offPeriod_msIn = offPeriod_msIn;
	ledIn->blink.r = rIn;
	ledIn->blink.g = gIn;
	ledIn->blink.b = bIn;

	// execute our first color
	oneShotCb_blinkOn((void*)ledIn);
}


static void scm_alternateColors(cxa_rgbLed_t *const superIn,
							   uint8_t r1In, uint8_t g1In, uint8_t b1In,
							   uint16_t color1Period_msIn,
							   uint8_t r2In, uint8_t g2In, uint8_t b2In,
							   uint16_t color2Period_msIn)
{
	cxa_rgbLed_pca9624_t* ledIn = (cxa_rgbLed_pca9624_t*)superIn;
	cxa_assert(ledIn);

	// brightness adjustment is done lazily (before actually setting RGB)

	// save our settings
	ledIn->alternate.color1Period_msIn = color1Period_msIn;
	ledIn->alternate.r1 = r1In;
	ledIn->alternate.g1 = g1In;
	ledIn->alternate.b1 = b1In;
	ledIn->alternate.color2Period_msIn = color2Period_msIn;
	ledIn->alternate.r2 = r2In;
	ledIn->alternate.g2 = g2In;
	ledIn->alternate.b2 = b2In;

	// execute our first color
	oneShotCb_alternate2((void*)ledIn);
}


static void scm_flashOnce(cxa_rgbLed_t *const superIn,
						  uint8_t rIn, uint8_t gIn, uint8_t bIn,
						  uint16_t period_msIn)
{
	cxa_rgbLed_pca9624_t* ledIn = (cxa_rgbLed_pca9624_t*)superIn;
	cxa_assert(ledIn);

	// brightness adjustment is done lazily (before actually setting RGB)

	// save our settings
	ledIn->flashOnce.prevR = ledIn->currColor.r;
	ledIn->flashOnce.prevG = ledIn->currColor.g;
	ledIn->flashOnce.prevB = ledIn->currColor.b;

	// set the new color
	scm_setRgb(&ledIn->super, rIn, gIn, bIn);

	// register our one-shot timer
	cxa_oneShotTimer_schedule(&ledIn->ost, period_msIn, oneShotCb_flashOnce, (void*)ledIn);
}


static void oneShotCb_blinkOn(void* userVarIn)
{
	cxa_rgbLed_pca9624_t* ledIn = (cxa_rgbLed_pca9624_t*)userVarIn;
	cxa_assert(ledIn);

	if( ledIn->super.currState != CXA_RGBLED_STATE_BLINK ) return;

	// turn off the LED
	scm_setRgb(&ledIn->super, 0, 0, 0);

	// register our one-shot timer
	cxa_oneShotTimer_schedule(&ledIn->ost, ledIn->blink.offPeriod_msIn, oneShotCb_blinkOff, (void*)ledIn);
}


static void oneShotCb_blinkOff(void* userVarIn)
{
	cxa_rgbLed_pca9624_t* ledIn = (cxa_rgbLed_pca9624_t*)userVarIn;
	cxa_assert(ledIn);

	if( ledIn->super.currState != CXA_RGBLED_STATE_BLINK ) return;

	// set our color
	scm_setRgb(&ledIn->super, ledIn->blink.r, ledIn->blink.g, ledIn->blink.b);

	// register our one-shot timer
	cxa_oneShotTimer_schedule(&ledIn->ost, ledIn->blink.onPeriod_msIn, oneShotCb_blinkOn, (void*)ledIn);
}


static void oneShotCb_alternate1(void* userVarIn)
{
	cxa_rgbLed_pca9624_t* ledIn = (cxa_rgbLed_pca9624_t*)userVarIn;
	cxa_assert(ledIn);

	if( ledIn->super.currState != CXA_RGBLED_STATE_ALTERNATE_COLORS ) return;

	// set our next color
	scm_setRgb(&ledIn->super, ledIn->alternate.r2, ledIn->alternate.g2, ledIn->alternate.b2);

	// register our one-shot timer
	cxa_oneShotTimer_schedule(&ledIn->ost, ledIn->alternate.color2Period_msIn, oneShotCb_alternate2, (void*)ledIn);
}


static void oneShotCb_alternate2(void* userVarIn)
{
	cxa_rgbLed_pca9624_t* ledIn = (cxa_rgbLed_pca9624_t*)userVarIn;
	cxa_assert(ledIn);

	if( ledIn->super.currState != CXA_RGBLED_STATE_ALTERNATE_COLORS ) return;

	// set our next color
	scm_setRgb(&ledIn->super, ledIn->alternate.r1, ledIn->alternate.g1, ledIn->alternate.b1);

	// register our one-shot timer
	cxa_oneShotTimer_schedule(&ledIn->ost, ledIn->alternate.color1Period_msIn, oneShotCb_alternate1, (void*)ledIn);
}


static void oneShotCb_flashOnce(void* userVarIn)
{
	cxa_rgbLed_pca9624_t* ledIn = (cxa_rgbLed_pca9624_t*)userVarIn;
	cxa_assert(ledIn);

	if( ledIn->super.currState != CXA_RGBLED_STATE_FLASHONCE ) return;

	// restore our previous state
	switch( ledIn->super.prevState )
	{
		case CXA_RGBLED_STATE_ALTERNATE_COLORS:
			ledIn->super.currState = CXA_RGBLED_STATE_ALTERNATE_COLORS;
			oneShotCb_alternate2((void*)ledIn);
			break;

		case CXA_RGBLED_STATE_BLINK:
			ledIn->super.currState = CXA_RGBLED_STATE_BLINK;
			oneShotCb_blinkOff((void*)ledIn);
			break;

		case CXA_RGBLED_STATE_SOLID:
			ledIn->super.currState = CXA_RGBLED_STATE_SOLID;
			scm_setRgb(&ledIn->super, ledIn->flashOnce.prevR, ledIn->flashOnce.prevG, ledIn->flashOnce.prevB);
			break;

		case CXA_RGBLED_STATE_FLASHONCE:
			// should never happen
			break;
	}
}
