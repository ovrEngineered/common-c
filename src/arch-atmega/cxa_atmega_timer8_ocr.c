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


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_atmega_timer8_ocr_configure(cxa_atmega_timer8_ocr_t *const ocrIn, const cxa_atmega_timer8_ocr_mode_t modeIn)
{
	cxa_assert(ocrIn);
	cxa_assert(ocrIn->parent);

	switch( ocrIn->parent->id )
	{
		case CXA_ATM_TIMER8_0:
			if( isChannelA(ocrIn) )
			{
				TCCR0A = (TCCR0A & ~(0x03 << 6)) | (modeIn << 6);
				DDRD = (DDRD & ~(1 << 6)) | (1 << 6);				// make sure to enable output drivers
			}
			else
			{
				TCCR0A = (TCCR0A & ~(0x03 << 4)) | (modeIn << 4);
				DDRD = (DDRD & ~(1 << 5)) | (1 << 5);				// make sure to enable output drivers
			}
			break;

		case CXA_ATM_TIMER8_1:
			if( isChannelA(ocrIn) )
			{
				TCCR1A = (TCCR1A & ~(0x03 << 6)) | (modeIn << 6);
				DDRB = (DDRB & ~(1 << 1)) | (1 << 1);				// make sure to enable output drivers
			}
			else
			{
				TCCR1A = (TCCR1A & ~(0x03 << 4)) | (modeIn << 4);
				DDRB = (DDRB & ~(1 << 2)) | (1 << 2);				// make sure to enable output drivers
			}
			break;

		case CXA_ATM_TIMER8_2:
			if( isChannelA(ocrIn) )
			{
				TCCR2A = (TCCR2A & ~(0x03 << 6)) | (modeIn << 6);
				DDRB = (DDRB & ~(1 << 3)) | (1 << 3);				// make sure to enable output drivers
			}
			else
			{
				TCCR2A = (TCCR2A & ~(0x03 << 4)) | (modeIn << 4);
				DDRD = (DDRD & ~(1 << 3)) | (1 << 3);				// make sure to enable output drivers
			}
			break;
	}
}


void cxa_atmega_timer8_ocr_setValue(cxa_atmega_timer8_ocr_t *const ocrIn, const uint8_t valueIn)
{
	cxa_assert(ocrIn);
	cxa_assert(ocrIn->parent);

	switch( ocrIn->parent->id )
	{
		case CXA_ATM_TIMER8_0:
			if( isChannelA(ocrIn) )
			{
				OCR0A = valueIn;
			}
			else
			{
				OCR0B = valueIn;
			}
			break;
			break;

		case CXA_ATM_TIMER8_1:
			if( isChannelA(ocrIn) )
			{
				OCR1A = valueIn;
			}
			else
			{
				OCR1B = valueIn;
			}
			break;

		case CXA_ATM_TIMER8_2:
			if( isChannelA(ocrIn) )
			{
				OCR2A = valueIn;
			}
			else
			{
				OCR2B = valueIn;
			}
			break;
	}
}


// ******** local function implementations ********
static bool isChannelA(cxa_atmega_timer8_ocr_t const* ocrIn)
{
	cxa_assert(ocrIn);

	return (ocrIn == &ocrIn->parent->ocrA);
}
