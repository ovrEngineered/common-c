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
#include "cxa_xmega_timer16.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <avr/io.h>
#include <cxa_assert.h>
#include <cxa_xmega_clockController.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_xmega_timer16_init_freerun(const cxa_xmega_timer16_t timerIn, cxa_xmega_timer16_clockSource_t clkSourceIn)
{
	cxa_assert( (timerIn == CXA_XMEGA_TIMER16_TCC0) ||
				(timerIn == CXA_XMEGA_TIMER16_TCC1) ||
				(timerIn == CXA_XMEGA_TIMER16_TCD0) ||
				(timerIn == CXA_XMEGA_TIMER16_TCD1) ||
				(timerIn == CXA_XMEGA_TIMER16_TCE0) ||
				(timerIn == CXA_XMEGA_TIMER16_TCE1) ||
				(timerIn == CXA_XMEGA_TIMER16_TCF0) );
				
	cxa_assert( (clkSourceIn == CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK) ||
				(clkSourceIn == CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK_DIV2) ||
				(clkSourceIn == CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK_DIV4) ||
				(clkSourceIn == CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK_DIV8) ||
				(clkSourceIn == CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK_DIV64) ||
				(clkSourceIn == CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK_DIV256) ||
				(clkSourceIn == CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK_DIV1024) ||
				(clkSourceIn == CXA_XMEGA_TIMER16_CLOCKSRC_EVENT_CHAN_0) ||
				(clkSourceIn == CXA_XMEGA_TIMER16_CLOCKSRC_EVENT_CHAN_1) ||
				(clkSourceIn == CXA_XMEGA_TIMER16_CLOCKSRC_EVENT_CHAN_2) ||
				(clkSourceIn == CXA_XMEGA_TIMER16_CLOCKSRC_EVENT_CHAN_3) ||
				(clkSourceIn == CXA_XMEGA_TIMER16_CLOCKSRC_EVENT_CHAN_4) ||
				(clkSourceIn == CXA_XMEGA_TIMER16_CLOCKSRC_EVENT_CHAN_5) ||
				(clkSourceIn == CXA_XMEGA_TIMER16_CLOCKSRC_EVENT_CHAN_6) ||
				(clkSourceIn == CXA_XMEGA_TIMER16_CLOCKSRC_EVENT_CHAN_7) );
				
	// that was a lot of asserts...
	
	// now ensure that power is enabled to our timer
	switch( timerIn )
	{
		case CXA_XMEGA_TIMER16_TCC0:
			PR.PRPC &= ~PR_TC0_bm;
			break;
			
		case CXA_XMEGA_TIMER16_TCC1:
			PR.PRPC &= ~PR_TC1_bm;
			break;
		
		case CXA_XMEGA_TIMER16_TCD0:
			PR.PRPD &= ~PR_TC0_bm;
			break;
		
		case CXA_XMEGA_TIMER16_TCD1:
			PR.PRPD &= ~PR_TC1_bm;
			break;
		
		case CXA_XMEGA_TIMER16_TCE0:
			PR.PRPE &= ~PR_TC0_bm;
			break;
		
		case CXA_XMEGA_TIMER16_TCE1:
			PR.PRPE &= ~PR_TC1_bm;
			break;
		
		case CXA_XMEGA_TIMER16_TCF0:
			PR.PRPF &= ~PR_TC0_bm;
			break;
		
	}
	
	// ensure we're not capturing or comparing AND that we are in 
	// "normal" waveform generation mode (with a maximum period)
	void *avrTc = cxa_xmega_timer16_getAvrTc(timerIn);
	cxa_assert(avrTc);
	
	((TC0_t*)avrTc)->CTRLB = 0x00;
	((TC0_t*)avrTc)->PER = 0xFFFF;
	
	// now set our clock source
	((TC0_t*)avrTc)->CTRLA = clkSourceIn;
}


uint16_t cxa_xmega_timer16_getCount(const cxa_xmega_timer16_t timerIn)
{
	cxa_assert( (timerIn == CXA_XMEGA_TIMER16_TCC0) ||
				(timerIn == CXA_XMEGA_TIMER16_TCC1) ||
				(timerIn == CXA_XMEGA_TIMER16_TCD0) ||
				(timerIn == CXA_XMEGA_TIMER16_TCD1) ||
				(timerIn == CXA_XMEGA_TIMER16_TCE0) ||
				(timerIn == CXA_XMEGA_TIMER16_TCE1) ||
				(timerIn == CXA_XMEGA_TIMER16_TCF0) );
	
	void *avrTc = cxa_xmega_timer16_getAvrTc(timerIn);
	cxa_assert(avrTc);
	
	return ((TC0_t*)avrTc)->CNT;
}


void* cxa_xmega_timer16_getAvrTc(const cxa_xmega_timer16_t timerIn)
{
	void *retVal = NULL;
	switch( timerIn )
	{
		case CXA_XMEGA_TIMER16_TCC0:
			retVal = (void*)&TCC0;
			break;
			
		case CXA_XMEGA_TIMER16_TCC1:
			retVal = (void*)&TCC1;
			break;
			
		case CXA_XMEGA_TIMER16_TCD0:
			retVal = (void*)&TCD0;
			break;
			
		case CXA_XMEGA_TIMER16_TCD1:
			retVal = (void*)&TCD1;
			break;
			
		case CXA_XMEGA_TIMER16_TCE0:
			retVal = (void*)&TCE0;
			break;
			
		case CXA_XMEGA_TIMER16_TCE1:
			retVal = (void*)&TCE1;
			break;
			
		case CXA_XMEGA_TIMER16_TCF0:
			retVal = (void*)&TCF0;
			break;
	}
		
	cxa_assert(retVal);
	return retVal;
}


uint32_t cxa_xmega_timer16_getResolution_cntsPerS(const cxa_xmega_timer16_t timerIn)
{
	cxa_assert( (timerIn == CXA_XMEGA_TIMER16_TCC0) ||
				(timerIn == CXA_XMEGA_TIMER16_TCC1) ||
				(timerIn == CXA_XMEGA_TIMER16_TCD0) ||
				(timerIn == CXA_XMEGA_TIMER16_TCD1) ||
				(timerIn == CXA_XMEGA_TIMER16_TCE0) ||
				(timerIn == CXA_XMEGA_TIMER16_TCE1) ||
				(timerIn == CXA_XMEGA_TIMER16_TCF0) );
	
	// figure out our prescaler
	void *avrTc = cxa_xmega_timer16_getAvrTc(timerIn);
	cxa_assert(avrTc);
	cxa_xmega_timer16_clockSource_t clkSrc = (cxa_xmega_timer16_clockSource_t)((TC0_t*)avrTc)->CTRLA & 0x0F;
	
	uint32_t retVal = cxa_xmega_clockController_getSystemClockFrequency_hz();
	switch(clkSrc)
	{
		case CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK:
			retVal = (retVal >> 0);
			break;
			
		case CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK_DIV2:
			retVal = (retVal >> 1);
			break;
		
		case CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK_DIV4:
			retVal = (retVal >> 2);
			break;
		
		case CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK_DIV8:
			retVal = (retVal >> 3);
			break;
		
		case CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK_DIV64:
			retVal = (retVal >> 6);
			break;
		
		case CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK_DIV256:
			retVal = (retVal >> 8);
			break;
		
		case CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK_DIV1024:
			retVal = (retVal >> 10);
			break;
		
		default:
			cxa_assert(0);
			break;
	}
	
	return retVal;
}


uint16_t cxa_xmega_timer16_getMaxVal_cnts(const cxa_xmega_timer16_t timerIn)
{
	cxa_assert( (timerIn == CXA_XMEGA_TIMER16_TCC0) ||
				(timerIn == CXA_XMEGA_TIMER16_TCC1) ||
				(timerIn == CXA_XMEGA_TIMER16_TCD0) ||
				(timerIn == CXA_XMEGA_TIMER16_TCD1) ||
				(timerIn == CXA_XMEGA_TIMER16_TCE0) ||
				(timerIn == CXA_XMEGA_TIMER16_TCE1) ||
				(timerIn == CXA_XMEGA_TIMER16_TCF0) );
				
	void *avrTc = cxa_xmega_timer16_getAvrTc(timerIn);
	cxa_assert(avrTc);
	
	return ((TC0_t*)avrTc)->PER;
}


// ******** local function implementations ********


