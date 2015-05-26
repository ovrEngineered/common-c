/**
 * Copyright 2013 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "cxa_xmega_timer32.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <avr/io.h>
#include <cxa_assert.h>
#include <cxa_xmega_eventSystem.h>
#include <cxa_xmega_clockController.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_xmega_timer32_init_freerun(cxa_xmega_timer32_t *const timerIn, const cxa_xmega_timer16_t timer_lowerIn, const cxa_xmega_timer16_t timer_upperIn, cxa_xmega_timer16_clockSource_t clkSourceIn)
{
	cxa_assert(timerIn);
	
	// save our references
	timerIn->timer_lower = timer_lowerIn;
	timerIn->timer_upper = timer_upperIn;
	
	// setup our lower timer
	cxa_xmega_timer16_init_freerun(timerIn->timer_lower, clkSourceIn);
	
	// setup our linking event channel
	cxa_xmega_eventSystem_eventChannel_t evChan_link = cxa_xmega_eventSystem_getUnusedEventChannel();
	cxa_xmega_eventSystem_initChannel_timerEvent(evChan_link, timerIn->timer_lower, CXA_XMEGA_EVENT_SOURCE_TC_OVERFLOW);
	
	// now setup our upper timer
	cxa_xmega_timer16_clockSource_t clkSrc = 0;
	switch( evChan_link )
	{
		case CXA_XMEGA_EVENT_CHAN_0:
			clkSrc = CXA_XMEGA_TIMER16_CLOCKSRC_EVENT_CHAN_0;
			break;
			
		case CXA_XMEGA_EVENT_CHAN_1:
			clkSrc = CXA_XMEGA_TIMER16_CLOCKSRC_EVENT_CHAN_1;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_2:
			clkSrc = CXA_XMEGA_TIMER16_CLOCKSRC_EVENT_CHAN_2;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_3:
			clkSrc = CXA_XMEGA_TIMER16_CLOCKSRC_EVENT_CHAN_3;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_4:
			clkSrc = CXA_XMEGA_TIMER16_CLOCKSRC_EVENT_CHAN_4;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_5:
			clkSrc = CXA_XMEGA_TIMER16_CLOCKSRC_EVENT_CHAN_5;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_6:
			clkSrc = CXA_XMEGA_TIMER16_CLOCKSRC_EVENT_CHAN_6;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_7:
			clkSrc = CXA_XMEGA_TIMER16_CLOCKSRC_EVENT_CHAN_7;
			break;
		
	}
	cxa_xmega_timer16_init_freerun(timerIn->timer_upper, clkSrc);
	
	// finally, setup our capture channels for synchronous reading
	cxa_xmega_eventSystem_eventChannel_t evChan_lowerCap = cxa_xmega_eventSystem_getUnusedEventChannel();
	cxa_xmega_eventSystem_initChannel_manualOnly(evChan_lowerCap);
	cxa_xmega_timer16_captureChannel_init(&timerIn->cc_lower, CXA_XMEGA_TIMER16_CAPTURE_CHANNEL_A, timerIn->timer_lower, evChan_lowerCap, false);
	cxa_xmega_eventSystem_eventChannel_t evChan_upperCap = cxa_xmega_eventSystem_getUnusedEventChannel();
	cxa_xmega_eventSystem_initChannel_manualOnly(evChan_upperCap);
	cxa_xmega_timer16_captureChannel_init(&timerIn->cc_upper, CXA_XMEGA_TIMER16_CAPTURE_CHANNEL_A, timerIn->timer_upper, evChan_upperCap, true);
}


uint32_t cxa_xmega_timer32_getCount(cxa_xmega_timer32_t *const timerIn)
{	
	// @TODO temporary fix to issue 9
	uint32_t retVal = 0;
	for( uint8_t i = 0; i < 2; i++ )
	{
		// trigger our capture events
		cxa_xmega_eventSystem_triggerEvents(cxa_xmega_timer16_captureChannel_getTriggerEventChannel(&timerIn->cc_lower) |
			cxa_xmega_timer16_captureChannel_getTriggerEventChannel(&timerIn->cc_upper));
	
		// now read our capture values
		retVal = (((uint32_t)cxa_xmega_timer16_captureChannel_getLastCaptureVal(&timerIn->cc_upper)) << 16) |
			((uint32_t)cxa_xmega_timer16_captureChannel_getLastCaptureVal(&timerIn->cc_lower));
	}
	
	return retVal;
}


uint32_t cxa_xmega_timer32_getResolution_cntsPerS(cxa_xmega_timer32_t *const timerIn)
{
	cxa_assert(timerIn);
	
	return cxa_xmega_timer16_getResolution_cntsPerS(timerIn->timer_lower);
}


uint32_t cxa_xmega_timer32_getMaxVal_cnts(cxa_xmega_timer32_t *const timerIn)
{
	cxa_assert(timerIn);
	
	return (((uint32_t)cxa_xmega_timer16_getMaxVal_cnts(timerIn->timer_upper)) << 16) |
			((uint32_t)cxa_xmega_timer16_getMaxVal_cnts(timerIn->timer_lower));
}


// ******** local function implementations ********

