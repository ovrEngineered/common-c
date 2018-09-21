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
#include "cxa_atmega_adcChannel.h"


// ******** includes ********
#include <avr/io.h>
#include <math.h>

#include <cxa_assert.h>


// ******** local macro definitions ********
#define ADPS_CLOCK_DIV				0x07


// ******** local type definitions ********


// ******** local function prototypes ********
static bool scm_startConversion_singleShot(cxa_adcChannel_t *const superIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_atmega_adcChannel_init(cxa_atmega_adcChannel_t* const adcChanIn, const cxa_atmega_adcChannel_id_t chanIdIn,
								const cxa_atmega_adcChannel_reference_t referenceIn)
{
	cxa_assert(adcChanIn);

	// save our references
	adcChanIn->chanId = chanIdIn;
	adcChanIn->reference = referenceIn;

	// disable digital input on this pin
	DIDR0 |= (1 << adcChanIn->chanId);

	// initialize our super class
	cxa_adcChannel_init(&adcChanIn->super, scm_startConversion_singleShot);
}


// ******** local function implementations ********
static bool scm_startConversion_singleShot(cxa_adcChannel_t *const superIn)
{
	cxa_atmega_adcChannel_t* adcChanIn = (cxa_atmega_adcChannel_t*)superIn;
	cxa_assert(superIn);

	ADMUX = (adcChanIn->reference << 6) | (adcChanIn->chanId << 0);
	ADCSRA = (1 << 7) | (ADPS_CLOCK_DIV << 0);

	// start the conversion and wait for it to finish
	ADCSRA |= (1 << 6);
	while( (ADCSRA & (1 << 6)) );

	// process our value
	uint16_t rawVal = ((uint16_t)ADCL << 0) | ((uint16_t)ADCH << 8);

	// notify our listeners
	cxa_adcChannel_notify_conversionComplete(&adcChanIn->super, NAN, rawVal);

	return true;
}
