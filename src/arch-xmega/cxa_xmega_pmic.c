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
#include "cxa_xmega_pmic.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********
static bool isInitialized = false;


// ******** global function implementations ********
void cxa_xmega_pmic_init(void)
{
	cli();
	PMIC.CTRL = 0;
	
	isInitialized = true;
}


void cxa_xmega_pmic_enableInterruptLevel(const cxa_xmega_pmic_interruptLevel_t levelIn)
{
	cxa_assert(isInitialized);
	
	switch( levelIn )
	{
		case CXA_XMEGA_PMIC_INTLEVEL_LOW:
			PMIC.CTRL |= PMIC_LOLVLEX_bm;
			break;
		
		case CXA_XMEGA_PMIC_INTLEVEL_MED:
			PMIC.CTRL |= PMIC_MEDLVLEX_bm;
			break;
			
		case CXA_XMEGA_PMIC_INTLEVEL_HIGH:
			PMIC.CTRL |= PMIC_HILVLEX_bm;
			break;
	}
}


void cxa_xmega_pmic_enableGlobalInterrupts(void)
{
	cxa_assert(isInitialized);
	sei();
}


void cxa_xmega_pmic_disableGlobalInterrupts(void)
{
	cxa_assert(isInitialized);
	cli();
}


// ******** local function implementations ********

