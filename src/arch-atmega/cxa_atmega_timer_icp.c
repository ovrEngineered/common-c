/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_atmega_timer_icp.h"


// ******** includes ********
#include <avr/io.h>
#include <cxa_assert.h>
#include <cxa_atmega_timer.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_atmega_timer_icp_configure(cxa_atmega_timer_icp_t *const icpIn, const cxa_atmega_timer_icp_edgeType_t edgeIn)
{
	cxa_assert(icpIn);
	cxa_assert(icpIn->parent->id == CXA_ATM_TIMER_1);

	TCCR1B = (TCCR1B & ~(1 << 6)) | (edgeIn << 6);
}


void cxa_atmega_timer_icp_swapEdge(cxa_atmega_timer_icp_t *const icpIn)
{
	cxa_assert(icpIn);
	cxa_assert(icpIn->parent->id == CXA_ATM_TIMER_1);

	TCCR1B ^= (1 << 6);
}


void cxa_atmega_timer_icp_addListener(cxa_atmega_timer_icp_t *const icpIn, cxa_atmega_timer_icp_cb_onInputCapture_t cb_onInputCaptureIn, void* userVarIn)
{
	cxa_assert(icpIn);
	cxa_assert(icpIn->parent->id == CXA_ATM_TIMER_1);

	cxa_atmega_timer_icp_listenerEntry_t newEntry = {
			.cb_onInputCapture = cb_onInputCaptureIn,
			.userVar = userVarIn
	};
	cxa_assert(cxa_array_append(&icpIn->listeners, &newEntry));
}


void cxa_atmega_timer_icp_enableInterrupt_inputCapture(cxa_atmega_timer_icp_t *const icpIn)
{
	cxa_assert(icpIn);
	cxa_assert(icpIn->parent->id == CXA_ATM_TIMER_1);

	TIFR1 |= (1 << 5);
	TIMSK1 |= (1 << 5);
}


uint16_t cxa_atmega_timer_icp_getValue(cxa_atmega_timer_icp_t *const icpIn)
{
	cxa_assert(icpIn);
	cxa_assert(icpIn->parent->id == CXA_ATM_TIMER_1);

	return ICR1;
}


void cxa_atmega_timer_icp_init(cxa_atmega_timer_icp_t *const icpIn, cxa_atmega_timer_t *const parentIn)
{
	cxa_assert(icpIn);

	// save our references
	icpIn->parent = parentIn;

	cxa_array_initStd(&icpIn->listeners, icpIn->listeners_raw);
}


void cxa_atmega_timer_icp_handleInterrupt_inputCapture(cxa_atmega_timer_icp_t *const icpIn)
{
	if( icpIn == NULL ) return;

	cxa_array_iterate(&icpIn->listeners, currListener, cxa_atmega_timer_icp_listenerEntry_t)
	{
		if( (currListener != NULL) && (currListener->cb_onInputCapture != NULL) ) currListener->cb_onInputCapture(icpIn, currListener->userVar);
	}
}


// ******** local function implementations ********
