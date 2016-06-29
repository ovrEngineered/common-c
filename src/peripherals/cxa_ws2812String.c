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
#include "cxa_ws2812String.h"


// ******** includes ********
#include <string.h>
#include <cxa_assert.h>
#include <cxa_numberUtils.h>


// ******** local macro definitions ********


// ******** local type definitions ********
typedef enum
{
	STATE_IDLE,
	STATE_WRITE_INDIV_PIX,
	STATE_PULSE_FADEIN,
	STATE_PULSE_FADEOUT,
}state_t;


// ******** local function prototypes ********
static void stateCb_writeIndivPix_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_pulseFadeXXX_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_pulseFadeIn_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_pulseFadeOut_state(cxa_stateMachine_t *const smIn, void *userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_ws2812String_init(cxa_ws2812String_t *const ws2812In, cxa_ws2812String_pixelBuffer_t* pixelBuffersIn, size_t numPixelBuffersIn,
						   cxa_ws2812String_scm_writeBytes_t scm_writeBytesIn)
{
	cxa_assert(ws2812In);
	cxa_assert(pixelBuffersIn != NULL);
	cxa_assert(numPixelBuffersIn > 0);

	// save our references
	ws2812In->pixelBuffers = pixelBuffersIn;
	ws2812In->numPixels = numPixelBuffersIn;
	ws2812In->scm_writeBytes = scm_writeBytesIn;

	// set zero values for our buffers
	memset(ws2812In->pixelBuffers, 0, (ws2812In->numPixels*sizeof(*ws2812In->pixelBuffers)));
	ws2812In->scm_writeBytes(ws2812In, ws2812In->pixelBuffers, ws2812In->numPixels);

	// setup our timeDiff
	cxa_timeDiff_init(&ws2812In->td_fade, false);

	// setup our state machine
	cxa_stateMachine_init(&ws2812In->stateMachine, "ws2812");
	cxa_stateMachine_addState(&ws2812In->stateMachine, STATE_IDLE, "idle", NULL, NULL, NULL, (void*)ws2812In);
	cxa_stateMachine_addState(&ws2812In->stateMachine, STATE_WRITE_INDIV_PIX, "writeIndivPix", stateCb_writeIndivPix_enter, NULL, NULL, (void*)ws2812In);
	cxa_stateMachine_addState(&ws2812In->stateMachine, STATE_PULSE_FADEIN, "pulseFadeIn", stateCb_pulseFadeXXX_enter, stateCb_pulseFadeIn_state, NULL, (void*)ws2812In);
	cxa_stateMachine_addState(&ws2812In->stateMachine, STATE_PULSE_FADEOUT, "pulseFadeOut", stateCb_pulseFadeXXX_enter, stateCb_pulseFadeOut_state, NULL, (void*)ws2812In);
	cxa_stateMachine_setInitialState(&ws2812In->stateMachine, STATE_IDLE);
}


void cxa_ws2812String_blank_now(cxa_ws2812String_t *const ws2812In)
{
	cxa_assert(ws2812In);

	memset(ws2812In->pixelBuffers, 0, (ws2812In->numPixels*sizeof(*ws2812In->pixelBuffers)));
	ws2812In->scm_writeBytes(ws2812In, ws2812In->pixelBuffers, ws2812In->numPixels);

	cxa_stateMachine_transition(&ws2812In->stateMachine, STATE_IDLE);
	cxa_stateMachine_update(&ws2812In->stateMachine);
}


void cxa_ws2812String_setPixel_rgb(cxa_ws2812String_t *const ws2812In, size_t pixelNumIn, uint8_t rIn, uint8_t gIn, uint8_t bIn)
{
	cxa_assert(ws2812In);
	cxa_assert(pixelNumIn < ws2812In->numPixels);

	ws2812In->pixelBuffers[pixelNumIn].r = rIn;
	ws2812In->pixelBuffers[pixelNumIn].g = gIn;
	ws2812In->pixelBuffers[pixelNumIn].b = bIn;

	cxa_stateMachine_transition(&ws2812In->stateMachine, STATE_WRITE_INDIV_PIX);
}


void cxa_ws2812String_setPixel_hsv(cxa_ws2812String_t *const ws2812In, size_t pixelNumIn, uint8_t hIn, uint8_t sIn, uint8_t vIn)
{
	cxa_assert(ws2812In);
	cxa_assert(pixelNumIn < ws2812In->numPixels);

	uint8_t region, remainder, p, q, t;
	uint8_t r, g, b;

	if( sIn == 0 )
	{
		r = vIn;
		g = vIn;
		b = vIn;
	}
	else
	{
		region = hIn / 43;
		remainder = (hIn - (region * 43)) * 6;

		p = (vIn * (255 - sIn)) >> 8;
		q = (vIn * (255 - ((sIn * remainder) >> 8))) >> 8;
		t = (vIn * (255 - ((sIn * (255 - remainder)) >> 8))) >> 8;

		switch (region)
		{
			case 0:
				r = vIn; g = t; b = p;
				break;
			case 1:
				r = q; g = vIn; b = p;
				break;
			case 2:
				r = p; g = vIn; b = t;
				break;
			case 3:
				r = p; g = q; b = vIn;
				break;
			case 4:
				r = t; g = p; b = vIn;
				break;
			default:
				r = vIn; g = p; b = q;
				break;
		}
	}

	cxa_ws2812String_setPixel_rgb(ws2812In, pixelNumIn, r, g, b);
}


void cxa_ws2812String_pulseColor_rgb(cxa_ws2812String_t *const ws2812In, uint32_t pulsePeriod_msIn, uint8_t rIn, uint8_t gIn, uint8_t bIn)
{
	cxa_assert(ws2812In);

	// save our references
	ws2812In->pulseColor.pulsePeriod_ms = pulsePeriod_msIn;
	ws2812In->pulseColor.targetColor.r = rIn;
	ws2812In->pulseColor.targetColor.g = gIn;
	ws2812In->pulseColor.targetColor.b = bIn;

	cxa_stateMachine_transition(&ws2812In->stateMachine, STATE_PULSE_FADEIN);
}


void cxa_ws2812String_update(cxa_ws2812String_t *const ws2812In)
{
	cxa_assert(ws2812In);

	cxa_stateMachine_update(&ws2812In->stateMachine);
}


// ******** local function implementations ********
static void stateCb_writeIndivPix_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_ws2812String_t* ws2812In = (cxa_ws2812String_t*)userVarIn;
	cxa_assert(ws2812In);

	// write all of our LED values
	cxa_assert(ws2812In->scm_writeBytes);
	ws2812In->scm_writeBytes(ws2812In, ws2812In->pixelBuffers, ws2812In->numPixels);

	// transition back to idle
	cxa_stateMachine_transition(&ws2812In->stateMachine, STATE_IDLE);
}


static void stateCb_pulseFadeXXX_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_ws2812String_t* ws2812In = (cxa_ws2812String_t*)userVarIn;
	cxa_assert(ws2812In);

	ws2812In->lastUpdateTime_ms = 0;
	cxa_timeDiff_setStartTime_now(&ws2812In->td_fade);

	// flush our current values to the LEDs
	cxa_assert(ws2812In->scm_writeBytes);
	ws2812In->scm_writeBytes(ws2812In, ws2812In->pixelBuffers, ws2812In->numPixels);
}


static void stateCb_pulseFadeIn_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_ws2812String_t* ws2812In = (cxa_ws2812String_t*)userVarIn;
	cxa_assert(ws2812In);

	// calculate some timing info
	uint32_t elapsedTime_ms = cxa_timeDiff_getElapsedTime_ms(&ws2812In->td_fade);
	uint32_t timeRemaining_ms = (ws2812In->pulseColor.pulsePeriod_ms / 2) - elapsedTime_ms;
	uint32_t deltaT_ms = elapsedTime_ms - ws2812In->lastUpdateTime_ms;
	uint32_t numStepsRemaining = CXA_MAX((timeRemaining_ms / deltaT_ms), 1);

	// set our new pixel values
	for( size_t i_currPixel = 0; i_currPixel < ws2812In->numPixels; i_currPixel++ )
	{
		cxa_ws2812String_pixelBuffer_t* currPixel = &ws2812In->pixelBuffers[i_currPixel];

		currPixel->r = CXA_MIN(currPixel->r + ((ws2812In->pulseColor.targetColor.r - currPixel->r) / numStepsRemaining), 255);
		currPixel->g = CXA_MIN(currPixel->g + ((ws2812In->pulseColor.targetColor.g - currPixel->g) / numStepsRemaining), 255);
		currPixel->b = CXA_MIN(currPixel->b + ((ws2812In->pulseColor.targetColor.b - currPixel->b) / numStepsRemaining), 255);
	}

	// write all of our LED values
	cxa_assert(ws2812In->scm_writeBytes);
	ws2812In->scm_writeBytes(ws2812In, ws2812In->pixelBuffers, ws2812In->numPixels);

	// see if it's time to fade out
	if( cxa_timeDiff_isElapsed_ms(&ws2812In->td_fade, ws2812In->pulseColor.pulsePeriod_ms/2) )
	{
		cxa_stateMachine_transition(&ws2812In->stateMachine, STATE_PULSE_FADEOUT);
		return;
	}
}


static void stateCb_pulseFadeOut_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_ws2812String_t* ws2812In = (cxa_ws2812String_t*)userVarIn;
	cxa_assert(ws2812In);

	// calculate some timing info
	uint32_t elapsedTime_ms = cxa_timeDiff_getElapsedTime_ms(&ws2812In->td_fade);
	uint32_t timeRemaining_ms = (ws2812In->pulseColor.pulsePeriod_ms / 2) - elapsedTime_ms;
	uint32_t deltaT_ms = elapsedTime_ms - ws2812In->lastUpdateTime_ms;
	uint32_t numStepsRemaining = (timeRemaining_ms / deltaT_ms);

	// set our new pixel values
	for( size_t i_currPixel = 0; i_currPixel < ws2812In->numPixels; i_currPixel++ )
	{
		cxa_ws2812String_pixelBuffer_t* currPixel = &ws2812In->pixelBuffers[i_currPixel];

		currPixel->r = ((currPixel->r / numStepsRemaining) <= currPixel->r) ? (currPixel->r - (currPixel->r / numStepsRemaining)) : 0;
		currPixel->g = ((currPixel->g / numStepsRemaining) <= currPixel->g) ? (currPixel->g - (currPixel->g / numStepsRemaining)) : 0;
		currPixel->b = ((currPixel->b / numStepsRemaining) <= currPixel->b) ? (currPixel->b - (currPixel->b / numStepsRemaining)) : 0;
	}

	// write all of our LED values
	cxa_assert(ws2812In->scm_writeBytes);
	ws2812In->scm_writeBytes(ws2812In, ws2812In->pixelBuffers, ws2812In->numPixels);

	// see if it's time to fade out
	if( cxa_timeDiff_isElapsed_ms(&ws2812In->td_fade, ws2812In->pulseColor.pulsePeriod_ms/2) )
	{
		cxa_stateMachine_transition(&ws2812In->stateMachine, STATE_PULSE_FADEIN);
		return;
	}
}
