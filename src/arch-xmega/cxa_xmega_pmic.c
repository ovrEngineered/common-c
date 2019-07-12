/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_xmega_pmic.h"


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


bool cxa_xmega_pmic_hasBeenInitialized(void)
{
	return isInitialized;
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
