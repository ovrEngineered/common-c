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
#include "cxa_atmega_timer8_ocr.h"


// ******** includes ********
#include <avr/io.h>
#include <stdbool.h>

#include <cxa_assert.h>
#include <cxa_atmega_timer8.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static bool isChannelA(cxa_atmega_timer8_ocr_t const* ocrIn);
static void setMode(cxa_atmega_timer8_ocr_t const* ocrIn, const cxa_atmega_timer8_ocr_mode_t modeIn);
static void enableOutputDrivers(cxa_atmega_timer8_ocr_t const* ocrIn, const bool driverEnableIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_atmega_timer8_ocr_configure(cxa_atmega_timer8_ocr_t *const ocrIn, const cxa_atmega_timer8_ocr_mode_t modeIn)
{
	cxa_assert(ocrIn);
	cxa_assert(ocrIn->parent);

	setMode(ocrIn, modeIn);
	cxa_atmega_timer8_ocr_setValue(ocrIn, 0);
}


void cxa_atmega_timer8_ocr_setValue(cxa_atmega_timer8_ocr_t *const ocrIn, const uint8_t valueIn)
{
	cxa_assert(ocrIn);
	cxa_assert(ocrIn->parent);

	uint8_t prevValue = 0;
	switch( ocrIn->parent->id )
	{
		case CXA_ATM_TIMER8_0:
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

		case CXA_ATM_TIMER8_1:
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

		case CXA_ATM_TIMER8_2:
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
	if( (prevValue > 0) && (valueIn == 0) ) enableOutputDrivers(ocrIn, false);
	if( (prevValue == 0) && (valueIn > 0) ) enableOutputDrivers(ocrIn, true);
}


// ******** local function implementations ********
static bool isChannelA(cxa_atmega_timer8_ocr_t const* ocrIn)
{
	cxa_assert(ocrIn);

	return (ocrIn == &ocrIn->parent->ocrA);
}


static void setMode(cxa_atmega_timer8_ocr_t const* ocrIn, const cxa_atmega_timer8_ocr_mode_t modeIn)
{
	cxa_assert(ocrIn);

	switch( ocrIn->parent->id )
	{
		case CXA_ATM_TIMER8_0:
			if( isChannelA(ocrIn) )
			{
				TCCR0A = (TCCR0A & ~(0x03 << 6)) | (modeIn << 6);
			}
			else
			{
				TCCR0A = (TCCR0A & ~(0x03 << 4)) | (modeIn << 4);
			}
			break;

		case CXA_ATM_TIMER8_1:
			if( isChannelA(ocrIn) )
			{
				TCCR1A = (TCCR1A & ~(0x03 << 6)) | (modeIn << 6);
			}
			else
			{
				TCCR1A = (TCCR1A & ~(0x03 << 4)) | (modeIn << 4);
			}
			break;

		case CXA_ATM_TIMER8_2:
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


static void enableOutputDrivers(cxa_atmega_timer8_ocr_t const* ocrIn, const bool driverEnableIn)
{
	cxa_assert(ocrIn);

	switch( ocrIn->parent->id )
	{
		case CXA_ATM_TIMER8_0:
			if( isChannelA(ocrIn) )
			{
				DDRD = (DDRD & ~(1 << 6)) | (driverEnableIn << 6);
			}
			else
			{
				DDRD = (DDRD & ~(1 << 5)) | (driverEnableIn << 5);
			}
			break;

		case CXA_ATM_TIMER8_1:
			if( isChannelA(ocrIn) )
			{
				DDRB = (DDRB & ~(1 << 1)) | (driverEnableIn << 1);
			}
			else
			{
				DDRB = (DDRB & ~(1 << 2)) | (driverEnableIn << 2);
			}
			break;

		case CXA_ATM_TIMER8_2:
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
