/**
 * @copyright 2018 opencxa.org
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
#include "cxa_atmega_timer8.h"


// ******** includes ********
#include <avr/io.h>
#include <avr/interrupt.h>

#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static uint16_t getPrescalerValueFromPrescaler(const cxa_atmega_timer8_prescaler_t prescaleIn);
static uint8_t getModeBitsFromTimerAndMode(const cxa_atmega_timer8_id_t idIn, const cxa_atmega_timer8_mode_t modeIn);
static uint8_t getPrescaleBitsTimerAndPrescale(const cxa_atmega_timer8_id_t idIn, const cxa_atmega_timer8_prescaler_t prescaleIn);
static void handleOverflowInterruptWithTimer(cxa_atmega_timer8_t *const t8In);


// ********  local variable declarations *********
static cxa_atmega_timer8_t* timer0 = NULL;
static cxa_atmega_timer8_t* timer1 = NULL;
static cxa_atmega_timer8_t* timer2 = NULL;


// ******** global function implementations ********
void cxa_atmega_timer8_init(cxa_atmega_timer8_t *const t8In, const cxa_atmega_timer8_id_t idIn, const cxa_atmega_timer8_mode_t modeIn, const cxa_atmega_timer8_prescaler_t prescaleIn)
{
	cxa_assert(t8In);

	// save our references and initialize our state
	t8In->id = idIn;
	t8In->prescaler = prescaleIn;
	cxa_array_initStd(&t8In->listeners, t8In->listeners_raw);

	// setup our output compare units
	t8In->ocrA.parent = t8In;
	t8In->ocrB.parent = t8In;

	uint8_t modeBits = getModeBitsFromTimerAndMode(idIn, modeIn);
	uint8_t prescaleBits = getPrescaleBitsTimerAndPrescale(idIn, prescaleIn);

	switch( t8In->id )
	{
		case CXA_ATM_TIMER8_0:
			TCCR0A = ((modeBits & 0x03) << 0);
			TCCR0B = (((modeBits >> 2) & 0x03) << 3) | (prescaleBits << 0);

			timer0 = t8In;			// save for interrupts
			break;

		case CXA_ATM_TIMER8_1:
			TCCR1A = ((modeBits & 0x03) << 0);
			TCCR1B = (((modeBits >> 2) & 0x03) << 3) | (prescaleBits << 0);

			timer1 = t8In;			// save for interrupts
			break;

		case CXA_ATM_TIMER8_2:
			TCCR2A = ((modeBits & 0x03) << 0);
			TCCR2B = (((modeBits >> 2) & 0x03) << 3) | (prescaleBits << 0);

			timer2 = t8In;			// save for interrupts
			break;
	}
}


void cxa_atmega_timer8_addListener(cxa_atmega_timer8_t *const t8In, cxa_atmega_timer8_cb_onOverflow_t cb_onOverflowIn, void* userVarIn)
{
	cxa_assert(t8In);

	cxa_atmega_timer8_listenerEntry_t newEntry = {
			.cb_onOverflow = cb_onOverflowIn,
			.userVar = userVarIn
	};
	cxa_assert(cxa_array_append(&t8In->listeners, &newEntry));
}


cxa_atmega_timer8_ocr_t* cxa_atmega_timer8_getOcrA(cxa_atmega_timer8_t const* t8In)
{
	cxa_assert(t8In);

	return (cxa_atmega_timer8_ocr_t*)&t8In->ocrA;
}


cxa_atmega_timer8_ocr_t* cxa_atmega_timer8_getOcrB(cxa_atmega_timer8_t const* t8In)
{
	cxa_assert(t8In);

	return (cxa_atmega_timer8_ocr_t*)&t8In->ocrB;
}


void cxa_atmega_timer8_enableInterrupt_overflow(cxa_atmega_timer8_t *const t8In)
{
	cxa_assert(t8In);

	switch( t8In->id )
	{
		case CXA_ATM_TIMER8_0:
			TIMSK0 |= (1 << 0);
			break;

		case CXA_ATM_TIMER8_1:
			TIMSK1 |= (1 << 0);
			break;

		case CXA_ATM_TIMER8_2:
			TIMSK2 |= (1 << 0);
			break;
	}
}


uint32_t cxa_atmega_timer8_getOverflowPeriod_us(cxa_atmega_timer8_t *const t8In)
{
	cxa_assert(t8In);

	return (UINT8_MAX * 1000UL * getPrescalerValueFromPrescaler(t8In->prescaler)) / (F_CPU / 1000UL);
}


// ******** local function implementations ********
static uint16_t getPrescalerValueFromPrescaler(const cxa_atmega_timer8_prescaler_t prescaleIn)
{
	uint16_t retVal = 0;
	switch( prescaleIn )
	{
		case CXA_ATM_TIMER8_PRESCALE_STOPPED:
			retVal = 0;
			break;

		case CXA_ATM_TIMER8_PRESCALE_1:
			retVal = 1;
			break;

		case CXA_ATM_TIMER8_PRESCALE_8:
			retVal = 8;
			break;

		case CXA_ATM_TIMER8_PRESCALE_32:
			retVal = 32;
			break;

		case CXA_ATM_TIMER8_PRESCALE_64:
			retVal = 64;
			break;

		case CXA_ATM_TIMER8_PRESCALE_128:
			retVal = 128;
			break;

		case CXA_ATM_TIMER8_PRESCALE_256:
			retVal = 256;
			break;

		case CXA_ATM_TIMER8_PRESCALE_1024:
			retVal = 1024;
			break;
	}
	return retVal;
}


static uint8_t getModeBitsFromTimerAndMode(const cxa_atmega_timer8_id_t idIn, const cxa_atmega_timer8_mode_t modeIn)
{
	uint8_t retVal = 0;
	switch( idIn )
	{
		case CXA_ATM_TIMER8_0:
			if( modeIn == CXA_ATM_TIMER8_MODE_FASTPWM ) retVal = 0x03;
			else cxa_assert(0);
			break;

		case CXA_ATM_TIMER8_1:
			if( modeIn == CXA_ATM_TIMER8_MODE_FASTPWM ) retVal = 0x05;
			else cxa_assert(0);
			break;

		case CXA_ATM_TIMER8_2:
			if( modeIn == CXA_ATM_TIMER8_MODE_FASTPWM ) retVal = 0x03;
			else cxa_assert(0);
			break;
	}
	return retVal;
}


static uint8_t getPrescaleBitsTimerAndPrescale(const cxa_atmega_timer8_id_t idIn, const cxa_atmega_timer8_prescaler_t prescaleIn)
{
	uint8_t retVal = 0;
	switch( prescaleIn )
	{
		case CXA_ATM_TIMER8_PRESCALE_STOPPED:
			if( idIn == CXA_ATM_TIMER8_0 ) retVal = 0;
			else if( idIn == CXA_ATM_TIMER8_1 ) retVal = 0;
			else if( idIn == CXA_ATM_TIMER8_2 ) retVal = 0;
			break;

		case CXA_ATM_TIMER8_PRESCALE_1:
			if( idIn == CXA_ATM_TIMER8_0 ) retVal = 1;
			else if( idIn == CXA_ATM_TIMER8_1 ) retVal = 1;
			else if( idIn == CXA_ATM_TIMER8_2 ) retVal = 1;
			break;

		case CXA_ATM_TIMER8_PRESCALE_8:
			if( idIn == CXA_ATM_TIMER8_0 ) retVal = 2;
			else if( idIn == CXA_ATM_TIMER8_1 ) retVal = 2;
			else if( idIn == CXA_ATM_TIMER8_2 ) retVal = 2;
			break;

		case CXA_ATM_TIMER8_PRESCALE_32:
			if( idIn == CXA_ATM_TIMER8_0 ) {cxa_assert(0);}
			else if( idIn == CXA_ATM_TIMER8_1 ) {cxa_assert(0);}
			else if( idIn == CXA_ATM_TIMER8_2 ) retVal = 3;
			break;

		case CXA_ATM_TIMER8_PRESCALE_64:
			if( idIn == CXA_ATM_TIMER8_0 ) retVal = 3;
			else if( idIn == CXA_ATM_TIMER8_1 ) retVal = 3;
			else if( idIn == CXA_ATM_TIMER8_2 ) retVal = 4;
			break;

		case CXA_ATM_TIMER8_PRESCALE_128:
			if( idIn == CXA_ATM_TIMER8_0 ) {cxa_assert(0);}
			else if( idIn == CXA_ATM_TIMER8_1 ) {cxa_assert(0);}
			else if( idIn == CXA_ATM_TIMER8_2 ) retVal = 5;
			break;

		case CXA_ATM_TIMER8_PRESCALE_256:
			if( idIn == CXA_ATM_TIMER8_0 ) retVal = 4;
			else if( idIn == CXA_ATM_TIMER8_1 ) retVal = 4;
			else if( idIn == CXA_ATM_TIMER8_2 ) retVal = 6;
			break;

		case CXA_ATM_TIMER8_PRESCALE_1024:
			if( idIn == CXA_ATM_TIMER8_0 ) retVal = 5;
			else if( idIn == CXA_ATM_TIMER8_1 ) retVal = 5;
			else if( idIn == CXA_ATM_TIMER8_2 ) retVal = 7;
			break;
	}
	return retVal;
}


static void handleOverflowInterruptWithTimer(cxa_atmega_timer8_t *const t8In)
{
	if( t8In == NULL ) return;

	cxa_array_iterate(&timer0->listeners, currListener, cxa_atmega_timer8_listenerEntry_t)
	{
		if( (currListener != NULL) && (currListener->cb_onOverflow != NULL) ) currListener->cb_onOverflow(t8In, currListener->userVar);
	}
}


ISR(TIMER0_OVF_vect)
{
	if( timer0 != NULL ) handleOverflowInterruptWithTimer(timer0);
}


ISR(TIMER1_OVF_vect)
{
	if( timer1 != NULL ) handleOverflowInterruptWithTimer(timer1);
}


ISR(TIMER2_OVF_vect)
{
	if( timer2 != NULL ) handleOverflowInterruptWithTimer(timer2);
}
