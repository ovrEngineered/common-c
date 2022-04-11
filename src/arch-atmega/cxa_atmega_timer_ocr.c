/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include <cxa_atmega_timer_ocr.h>


// ******** includes *******
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>

#include <cxa_assert.h>
#include <cxa_atmega_timer.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static bool isChannelA(cxa_atmega_timer_ocr_t const* ocrIn);
static void setMode(cxa_atmega_timer_ocr_t const* ocrIn, const cxa_atmega_timer_ocr_mode_t modeIn);
static void enableOutputDrivers(cxa_atmega_timer_ocr_t const* ocrIn, const bool driverEnableIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_atmega_timer_ocr_configure(cxa_atmega_timer_ocr_t *const ocrIn, const cxa_atmega_timer_ocr_mode_t modeIn)
{
	cxa_assert(ocrIn);
	cxa_assert(ocrIn->parent);

	ocrIn->mode = modeIn;
	setMode(ocrIn, modeIn);

	cxa_atmega_timer_ocr_setValue(ocrIn, 0);
}


uint16_t cxa_atmega_timer_ocr_getValue(cxa_atmega_timer_ocr_t *const ocrIn)
{
	cxa_assert(ocrIn);
	cxa_assert(ocrIn->parent);

	uint16_t retVal = 0;
	switch( ocrIn->parent->id )
	{
		case CXA_ATM_TIMER_0:
			if( isChannelA(ocrIn) )
			{
				retVal = OCR0A;
			}
			else
			{
				retVal = OCR0B;
			}
			break;

		case CXA_ATM_TIMER_1:
			if( isChannelA(ocrIn) )
			{
				retVal = OCR1A;
			}
			else
			{
				retVal = OCR1B;
			}
			break;

		case CXA_ATM_TIMER_2:
			if( isChannelA(ocrIn) )
			{
				retVal = OCR2A;
			}
			else
			{
				retVal = OCR2B;
			}
			break;
	}

	return retVal;
}


void cxa_atmega_timer_ocr_setValue(cxa_atmega_timer_ocr_t *const ocrIn, const uint16_t valueIn)
{
	cxa_assert(ocrIn);
	cxa_assert(ocrIn->parent);

	uint8_t prevValue = 0;
	switch( ocrIn->parent->id )
	{
		case CXA_ATM_TIMER_0:
			if( isChannelA(ocrIn) )
			{
				prevValue = OCR0A;
				OCR0A = valueIn;
			}
			else
			{
				prevValue = OCR0B;
				OCR0B = valueIn;
			}
			break;

		case CXA_ATM_TIMER_1:
			if( isChannelA(ocrIn) )
			{
				prevValue = OCR1A;
				OCR1A = valueIn;
			}
			else
			{
				prevValue = OCR1B;
				OCR1B = valueIn;
			}
			break;

		case CXA_ATM_TIMER_2:
			if( isChannelA(ocrIn) )
			{
				prevValue = OCR2A;
				OCR2A = valueIn;
			}
			else
			{
				prevValue = OCR2B;
				OCR2B = valueIn;
			}
			break;
	}

	// enable or disable our output drivers as needed
	if( ocrIn->mode != CXA_ATM_TIMER_OCR_MODE_DISCONNECTED )
	{
		if( (prevValue > 0) && (valueIn == 0) ) enableOutputDrivers(ocrIn, false);
		if( (prevValue == 0) && (valueIn > 0) ) enableOutputDrivers(ocrIn, true);
	}
}


void cxa_atmega_timer_ocr_addListener(cxa_atmega_timer_ocr_t *const ocrIn, cxa_atmega_timer_ocr_cb_onCompareMatch_t cb_onCompareMatchIn, void* userVarIn)
{
	cxa_assert(ocrIn);

	cxa_atmega_timer_ocr_listenerEntry_t newEntry = {
			.cb_onCompareMatch = cb_onCompareMatchIn,
			.userVar = userVarIn
	};
	cxa_assert(cxa_array_append(&ocrIn->listeners, &newEntry));
}


void cxa_atmega_timer_ocr_enableInterrupt_compareMatch(cxa_atmega_timer_ocr_t *const ocrIn)
{
	cxa_assert(ocrIn);
	cxa_assert(ocrIn->parent);

	cli();
	switch( ocrIn->parent->id )
	{
		case CXA_ATM_TIMER_0:
			TIFR0 |= (1 << 1);
			if( isChannelA(ocrIn) )
			{
				TIMSK0 |= (1 << 1);
			}
			else
			{
				TIMSK0 |= (1 << 2);
			}
			break;

		case CXA_ATM_TIMER_1:
			TIFR1 |= (1 << 1);
			if( isChannelA(ocrIn) )
			{
				TIMSK1 |= (1 << 1);
			}
			else
			{
				TIMSK1 |= (1 << 2);
			}
			break;

		case CXA_ATM_TIMER_2:
			TIFR2 |= (1 << 1);
			if( isChannelA(ocrIn) )
			{
				TIMSK2 |= (1 << 1);
			}
			else
			{
				TIMSK2 |= (1 << 2);
			}
			break;
	}
	sei();
}


void cxa_atmega_timer_ocr_disableInterrupt_compareMatch(cxa_atmega_timer_ocr_t *const ocrIn)
{
	cxa_assert(ocrIn);
	cxa_assert(ocrIn->parent);

	cli();
	switch( ocrIn->parent->id )
	{
		case CXA_ATM_TIMER_0:
			if( isChannelA(ocrIn) )
			{
				TIMSK0 &= ~(1 << 1);
			}
			else
			{
				TIMSK0 &= ~(1 << 2);
			}
			break;

		case CXA_ATM_TIMER_1:
			if( isChannelA(ocrIn) )
			{
				TIMSK1 &= ~(1 << 1);
			}
			else
			{
				TIMSK1 &= ~(1 << 2);
			}
			break;

		case CXA_ATM_TIMER_2:
			if( isChannelA(ocrIn) )
			{
				TIMSK2 &= ~(1 << 1);
			}
			else
			{
				TIMSK2 &= ~(1 << 2);
			}
			break;
	}
	sei();
}


void cxa_atmega_timer_ocr_init(cxa_atmega_timer_ocr_t *const ocrIn, cxa_atmega_timer_t *const parentIn)
{
	cxa_assert(ocrIn);

	// save our references
	ocrIn->parent = parentIn;

	cxa_array_initStd(&ocrIn->listeners, ocrIn->listeners_raw);
}


void cxa_atmega_timer_ocr_handleInterrupt_compareMatch(cxa_atmega_timer_ocr_t *const ocrIn)
{
	if( ocrIn == NULL ) return;

	cxa_array_iterate(&ocrIn->listeners, currListener, cxa_atmega_timer_ocr_listenerEntry_t)
	{
		if( (currListener != NULL) && (currListener->cb_onCompareMatch != NULL) ) currListener->cb_onCompareMatch(ocrIn, currListener->userVar);
	}
}


// ******** local function implementations ********
static bool isChannelA(cxa_atmega_timer_ocr_t const* ocrIn)
{
	cxa_assert(ocrIn);

	return (ocrIn == &ocrIn->parent->ocrA);
}


static void setMode(cxa_atmega_timer_ocr_t const* ocrIn, const cxa_atmega_timer_ocr_mode_t modeIn)
{
	cxa_assert(ocrIn);

	switch( ocrIn->parent->id )
	{
		case CXA_ATM_TIMER_0:
			if( isChannelA(ocrIn) )
			{
				TCCR0A = (TCCR0A & ~(0x03 << 6)) | (modeIn << 6);
			}
			else
			{
				TCCR0A = (TCCR0A & ~(0x03 << 4)) | (modeIn << 4);
			}
			break;

		case CXA_ATM_TIMER_1:
			if( isChannelA(ocrIn) )
			{
				TCCR1A = (TCCR1A & ~(0x03 << 6)) | (modeIn << 6);
			}
			else
			{
				TCCR1A = (TCCR1A & ~(0x03 << 4)) | (modeIn << 4);
			}
			break;

		case CXA_ATM_TIMER_2:
			if( isChannelA(ocrIn) )
			{
				TCCR2A = (TCCR2A & ~(0x03 << 6)) | (modeIn << 6);
			}
			else
			{
				TCCR2A = (TCCR2A & ~(0x03 << 4)) | (modeIn << 4);
			}
			break;
	}
}


static void enableOutputDrivers(cxa_atmega_timer_ocr_t const* ocrIn, const bool driverEnableIn)
{
	cxa_assert(ocrIn);

	switch( ocrIn->parent->id )
	{
		case CXA_ATM_TIMER_0:
			if( isChannelA(ocrIn) )
			{
				DDRD = (DDRD & ~(1 << 6)) | (driverEnableIn << 6);
			}
			else
			{
				DDRD = (DDRD & ~(1 << 5)) | (driverEnableIn << 5);
			}
			break;

		case CXA_ATM_TIMER_1:
			if( isChannelA(ocrIn) )
			{
				DDRB = (DDRB & ~(1 << 1)) | (driverEnableIn << 1);
			}
			else
			{
				DDRB = (DDRB & ~(1 << 2)) | (driverEnableIn << 2);
			}
			break;

		case CXA_ATM_TIMER_2:
			if( isChannelA(ocrIn) )
			{
				DDRB = (DDRB & ~(1 << 3)) | (driverEnableIn << 3);
			}
			else
			{
				DDRD = (DDRD & ~(1 << 3)) | (driverEnableIn << 3);
			}
			break;
	}
}
