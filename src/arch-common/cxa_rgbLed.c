/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_rgbLed.h"


// ******** includes ********
#include <cxa_assert.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define SCALE_COLOR(colorIn)		(colorIn) = ((colorIn) * (ledIn->maxBright_pcnt100 / 100.0))


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_rgbLed_init(cxa_rgbLed_t *const ledIn,
					 cxa_rgbLed_scm_setRgb_t scm_setRgbIn,
					 cxa_rgbLed_scm_alternateColors_t scm_alternateColorsIn,
					 cxa_rgbLed_scm_flashOnce_t scm_flashOnceIn)
{
	cxa_assert(ledIn);
	cxa_assert(scm_setRgbIn);

	// save our references
	ledIn->scms.setRgb = scm_setRgbIn;
	ledIn->scms.alternateColors = scm_alternateColorsIn;
	ledIn->scms.flashOnce = scm_flashOnceIn;

	ledIn->currState = CXA_RGBLED_STATE_SOLID;
	ledIn->maxBright_pcnt100 = 100;
}


void cxa_rgbLed_setMaxBright_pcnt100(cxa_rgbLed_t *const ledIn, const uint8_t max_pcnt100In)
{
	cxa_assert(ledIn);

	ledIn->maxBright_pcnt100 = max_pcnt100In;
}


void cxa_rgbLed_setRgb(cxa_rgbLed_t *const ledIn, uint8_t rIn, uint8_t gIn, uint8_t bIn)
{
	cxa_assert(ledIn);

	ledIn->prevState = ledIn->currState;
	ledIn->currState = CXA_RGBLED_STATE_SOLID;

	SCALE_COLOR(rIn);
	SCALE_COLOR(gIn);
	SCALE_COLOR(bIn);

	ledIn->scms.setRgb(ledIn, rIn, gIn, bIn);
}


void cxa_rgbLed_blink(cxa_rgbLed_t *const ledIn, uint8_t rIn, uint8_t gIn, uint8_t bIn,
					  uint16_t onPeriod_msIn, uint16_t offPeriod_msIn)
{
	cxa_assert(ledIn);

	cxa_rgbLed_alternateColors(ledIn, rIn, gIn, bIn, onPeriod_msIn, 0, 0, 0, offPeriod_msIn);
}


void cxa_rgbLed_alternateColors(cxa_rgbLed_t *const ledIn,
							   uint8_t r1In, uint8_t g1In, uint8_t b1In,
							   uint16_t color1Period_msIn,
							   uint8_t r2In, uint8_t g2In, uint8_t b2In,
							   uint16_t color2Period_msIn)
{
	cxa_assert(ledIn);
	cxa_assert(ledIn->scms.alternateColors);

	ledIn->prevState = ledIn->currState;
	ledIn->currState = CXA_RGBLED_STATE_ALTERNATE_COLORS;

	SCALE_COLOR(r1In);
	SCALE_COLOR(g1In);
	SCALE_COLOR(b1In);

	SCALE_COLOR(r2In);
	SCALE_COLOR(g2In);
	SCALE_COLOR(b2In);

	ledIn->scms.alternateColors(ledIn, r1In, g1In, b1In, color1Period_msIn, r2In, g2In, b2In, color2Period_msIn);
}


void cxa_rgbLed_flashOnce(cxa_rgbLed_t *const ledIn, uint8_t rIn, uint8_t gIn, uint8_t bIn,
						  uint16_t period_msIn)
{
	cxa_assert(ledIn);
	cxa_assert(ledIn->scms.flashOnce);
	if( ledIn->currState == CXA_RGBLED_STATE_FLASHONCE) return;

	ledIn->prevState = ledIn->currState;
	ledIn->currState = CXA_RGBLED_STATE_FLASHONCE;

	SCALE_COLOR(rIn);
		SCALE_COLOR(gIn);
		SCALE_COLOR(bIn);

	ledIn->scms.flashOnce(ledIn, rIn, gIn, bIn, period_msIn);
}

// ******** local function implementations ********
