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
#include "cxa_xmega_timer16_captureChannel.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <avr/io.h>
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static uint8_t getEventSource_fromEventChan(const cxa_xmega_eventSystem_eventChannel_t chanIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_xmega_timer16_captureChannel_init(cxa_xmega_timer16_captureChannel_t *const ccIn, const cxa_xmega_timer16_captureChannel_enum_t chanIn, const cxa_xmega_timer16_t timerIn, const cxa_xmega_eventSystem_eventChannel_t capTriggerIn, bool evDelayIn)
{
	cxa_assert(ccIn);
	cxa_assert( (chanIn == CXA_XMEGA_TIMER16_CAPTURE_CHANNEL_A) ||
				(chanIn == CXA_XMEGA_TIMER16_CAPTURE_CHANNEL_B) ||
				(chanIn == CXA_XMEGA_TIMER16_CAPTURE_CHANNEL_C) ||
				(chanIn == CXA_XMEGA_TIMER16_CAPTURE_CHANNEL_D) );
				
	// save our references
	ccIn->enumChan = chanIn;
	ccIn->timer = timerIn;
	ccIn->triggerEventChan = capTriggerIn;
	
	// get a pointer to our register of interest
	void *avrTc = cxa_xmega_timer16_getAvrTc(ccIn->timer);
	cxa_assert(avrTc);
	
	// figure out if we are type 0 or type 1
	bool isType0 = (avrTc == &TCC0) | (avrTc == &TCD0) | (avrTc == &TCE0) | (avrTc == &TCF0);
	
	// setup our event actions
	((TC0_t*)avrTc)->CTRLD = TC_EVACT_CAPT_gc | (evDelayIn << 4) | (0x08 | getEventSource_fromEventChan(ccIn->triggerEventChan));
	
	// enable the capture
	switch(ccIn->enumChan)
	{
		case CXA_XMEGA_TIMER16_CAPTURE_CHANNEL_A:
			// clear our capture buffer and register first
			// this is super important because of the double buffering
			((TC0_t*)avrTc)->CCA = 0;
			((TC0_t*)avrTc)->CCABUF = 0;
			((TC0_t*)avrTc)->CTRLB |= TC0_CCAEN_bm;
			break;
			
		case CXA_XMEGA_TIMER16_CAPTURE_CHANNEL_B:
			// clear our capture buffer and register first
			// this is super important because of the double buffering
			((TC0_t*)avrTc)->CCB = 0;
			((TC0_t*)avrTc)->CCBBUF = 0;
			((TC0_t*)avrTc)->CTRLB |= TC0_CCBEN_bm;
			break;
		
		case CXA_XMEGA_TIMER16_CAPTURE_CHANNEL_C:
			cxa_assert(isType0);
			// clear our capture buffer and register first
			// this is super important because of the double buffering
			((TC0_t*)avrTc)->CCC = 0;
			((TC0_t*)avrTc)->CCCBUF = 0;
			((TC0_t*)avrTc)->CTRLB |= TC0_CCCEN_bm;
			break;
		
		case CXA_XMEGA_TIMER16_CAPTURE_CHANNEL_D:
			cxa_assert(isType0);
			// clear our capture buffer and register first
			// this is super important because of the double buffering
			((TC0_t*)avrTc)->CCD = 0;
			((TC0_t*)avrTc)->CCDBUF = 0;
			((TC0_t*)avrTc)->CTRLB |= TC0_CCDEN_bm;
			break;
	}
}


cxa_xmega_eventSystem_eventChannel_t cxa_xmega_timer16_captureChannel_getTriggerEventChannel(cxa_xmega_timer16_captureChannel_t *const ccIn)
{
	cxa_assert(ccIn);
	
	return ccIn->triggerEventChan;
}


uint16_t cxa_xmega_timer16_captureChannel_getLastCaptureVal(cxa_xmega_timer16_captureChannel_t *const ccIn)
{
	cxa_assert(ccIn);
	
	void *avrTc = cxa_xmega_timer16_getAvrTc(ccIn->timer);
	cxa_assert(avrTc);
	
	uint16_t retVal = 0;
	switch(ccIn->enumChan)
	{
		case CXA_XMEGA_TIMER16_CAPTURE_CHANNEL_A:
			retVal = ((TC0_t*)avrTc)->CCA;
			break;
		
		case CXA_XMEGA_TIMER16_CAPTURE_CHANNEL_B:
			retVal = ((TC0_t*)avrTc)->CCB;
			break;
		
		case CXA_XMEGA_TIMER16_CAPTURE_CHANNEL_C:
			retVal = ((TC0_t*)avrTc)->CCC;
			break;
		
		case CXA_XMEGA_TIMER16_CAPTURE_CHANNEL_D:
			retVal = ((TC0_t*)avrTc)->CCD;
			break;
	}
	
	return retVal;
}


// ******** local function implementations ********
static uint8_t getEventSource_fromEventChan(const cxa_xmega_eventSystem_eventChannel_t chanIn)
{
	uint8_t retVal = 0;
	switch(chanIn)
	{
		case CXA_XMEGA_EVENT_CHAN_0:
			retVal = 0;
			break;
			
		case CXA_XMEGA_EVENT_CHAN_1:
			retVal = 1;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_2:
			retVal = 2;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_3:
			retVal = 3;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_4:
			retVal = 4;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_5:
			retVal = 5;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_6:
			retVal = 6;
			break;
		
		case CXA_XMEGA_EVENT_CHAN_7:
			retVal = 7;
			break;
		
		default:
			cxa_assert(0);
			break;
	}
	
	return retVal;
}