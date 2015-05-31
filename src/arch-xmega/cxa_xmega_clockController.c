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
#include "cxa_xmega_clockController.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdbool.h>
#include <avr/io.h>
#include <cxa_assert.h>
#include <cxa_criticalSection.h>
#include <cxa_xmega_ccp.h>
#include <cxa_xmega_pmic.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_xmega_clockController_init(const cxa_xmega_clockController_internalOsc_t oscIn)
{
	// don't assume we can initialize this (since it may have side-effects)
	cxa_assert(cxa_xmega_pmic_hasBeenInitialized());
	
	// disable power to all of our peripherals 
	// (we'll re-enable it as we enable each peripheral)
	PR.PRGEN = 0xFF;
	PR.PRPA = 0xFF;
	PR.PRPB = 0xFF;
	PR.PRPC = 0xFF;
	PR.PRPD = 0xFF;
	PR.PRPE = 0xFF;
	PR.PRPF = 0xFF;
	
	// set our internal clock source
	cxa_xmega_clockController_setSystemClockSource_internal(oscIn);
}


void cxa_xmega_clockController_setSystemClockSource_internal(const cxa_xmega_clockController_internalOsc_t oscIn)
{
	cxa_assert( (oscIn == CXA_XMEGA_CC_INTOSC_32MHZ) ||
				(oscIn == CXA_XMEGA_CC_INTOSC_2MHZ) ||
				(oscIn == CXA_XMEGA_CC_INTOSC_32KHZ) );

	// figure out which oscillators were enabled...if only one was,
	// we're probably not doing anything crazy, so disable the old
	// oscillator when we're finished
	bool disableOldOsc = false;
	cxa_xmega_clockController_internalOsc_t oscToDisable;
	if( (OSC.CTRL & (1 << CXA_XMEGA_CC_INTOSC_32MHZ)) &&
		!(OSC.CTRL & (1 << CXA_XMEGA_CC_INTOSC_2MHZ)) &&
		!(OSC.CTRL & (1 << CXA_XMEGA_CC_INTOSC_32KHZ)) )
	{
		oscToDisable = CXA_XMEGA_CC_INTOSC_32MHZ;
		disableOldOsc = true;
	}
	else if( !(OSC.CTRL & (1 << CXA_XMEGA_CC_INTOSC_32MHZ)) &&
			  (OSC.CTRL & (1 << CXA_XMEGA_CC_INTOSC_2MHZ)) &&
			 !(OSC.CTRL & (1 << CXA_XMEGA_CC_INTOSC_32KHZ)) )
	{
		oscToDisable = CXA_XMEGA_CC_INTOSC_2MHZ;
		disableOldOsc = true;
	}
	else if( !(OSC.CTRL & (1 << CXA_XMEGA_CC_INTOSC_32MHZ)) &&
			 !(OSC.CTRL & (1 << CXA_XMEGA_CC_INTOSC_2MHZ)) &&
			  (OSC.CTRL & (1 << CXA_XMEGA_CC_INTOSC_32KHZ)) )
	{
		oscToDisable = CXA_XMEGA_CC_INTOSC_32KHZ;
		disableOldOsc = true;
	}

	// enable the selected oscillator
	cxa_xmega_clockController_internalOsc_enable(oscIn);

	// allow configuration change, then set the clock source
	cxa_xmega_ccp_writeIo((void*)&CLK.CTRL, oscIn);
	
	// disable our old clock if we can (to save some power)
	if( disableOldOsc ) cxa_xmega_clockController_internalOsc_disable(oscToDisable);
}


uint32_t cxa_xmega_clockController_getSystemClockFrequency_hz(void)
{
	// we only support internal clocks at this point...
	cxa_assert( (CLK.CTRL == CXA_XMEGA_CC_INTOSC_32MHZ) || 
				(CLK.CTRL == CXA_XMEGA_CC_INTOSC_2MHZ) ||
				(CLK.CTRL == CXA_XMEGA_CC_INTOSC_32KHZ) );
	
	uint32_t retVal = 0;
	switch( CLK.CTRL )
	{
		case CXA_XMEGA_CC_INTOSC_32MHZ:
			retVal = 32000000;
			break;
		
		case CXA_XMEGA_CC_INTOSC_2MHZ:
			retVal = 2000000;
			break;
		
		case CXA_XMEGA_CC_INTOSC_32KHZ:
			retVal = 32000;
			break;
			
		default:
			retVal = 0;
			break;
	}
	
	return retVal;
}


void cxa_xmega_clockController_internalOsc_enable(const cxa_xmega_clockController_internalOsc_t oscIn)
{
	cxa_criticalSection_enter();
	
	OSC.CTRL |= (1 << oscIn);
	while( !(OSC.STATUS | (1 << oscIn)) );
	
	cxa_criticalSection_exit();
}


void cxa_xmega_clockController_internalOsc_disable(const cxa_xmega_clockController_internalOsc_t oscIn)
{
	cxa_criticalSection_enter();
	
	OSC.CTRL &= ~(1 << oscIn);
	
	cxa_criticalSection_exit();
}


// ******** local function implementations ********

