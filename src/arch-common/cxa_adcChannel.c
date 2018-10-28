/**
 * @copyright 2015 opencxa.org
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
#include "cxa_adcChannel.h"


// ******** includes ********
#include <cxa_assert.h>
#include <math.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_adcChannel_init(cxa_adcChannel_t* const adcChanIn,
						 cxa_adcChannel_scm_startConversion_singleShot_t scm_startConv_ssIn,
						 cxa_adcChannel_scm_getMaxRawValue_t scm_getMaxRawValueIn)
{
	cxa_assert(adcChanIn);
	cxa_assert(scm_startConv_ssIn);
	cxa_assert(scm_getMaxRawValueIn);

	// save our references
	adcChanIn->scms.startConv_ss = scm_startConv_ssIn;
	adcChanIn->scms.getMaxRawValue = scm_getMaxRawValueIn;

	// setup our listeners
	cxa_array_initStd(&adcChanIn->listeners, adcChanIn->listeners_raw);

	adcChanIn->lastConversionValue.raw = 0;
	adcChanIn->lastConversionValue.voltage = NAN;
	adcChanIn->lastConversionValue.wasSuccessful = false;
}


void cxa_adcChannel_addListener(cxa_adcChannel_t *const adcChanIn,
						 cxa_adcChannel_cb_conversionComplete_t cb_convCompIn,
						 void* userVarIn)
{
	cxa_assert(adcChanIn);
	cxa_assert( cb_convCompIn != NULL );

	cxa_adcChannel_listener_t newListener =
	{
			.cb_convComp = cb_convCompIn,
			.userVar = userVarIn
	};

	cxa_assert(cxa_array_append(&adcChanIn->listeners, &newListener));
}


void cxa_adcChannel_startConversion_singleShot(cxa_adcChannel_t *const adcChanIn)
{
	cxa_assert(adcChanIn);

	adcChanIn->scms.startConv_ss(adcChanIn);
}


bool cxa_adcChannel_wasLastConversionSuccessful(cxa_adcChannel_t *const adcChanIn)
{
	cxa_assert(adcChanIn);

	return adcChanIn->lastConversionValue.wasSuccessful;
}


float cxa_adcChannel_getLastConversionValue_voltage(cxa_adcChannel_t *const adcChanIn)
{
	cxa_assert(adcChanIn);

	return adcChanIn->lastConversionValue.voltage;
}


uint16_t cxa_adcChannel_getLastConversionValue_raw(cxa_adcChannel_t *const adcChanIn)
{
	cxa_assert(adcChanIn);

	return adcChanIn->lastConversionValue.raw;
}


uint16_t cxa_adcChannel_getMaxRawValue(cxa_adcChannel_t *const adcChanIn)
{
	cxa_assert(adcChanIn);

	return adcChanIn->scms.getMaxRawValue(adcChanIn);
}


void cxa_adcChannel_notify_conversionComplete(cxa_adcChannel_t *const adcChanIn, bool wasSuccessfulIn, float voltageIn, const uint16_t rawValIn)
{
	cxa_assert(adcChanIn);

	// save internally
	adcChanIn->lastConversionValue.wasSuccessful = wasSuccessfulIn;
	adcChanIn->lastConversionValue.voltage = voltageIn;
	adcChanIn->lastConversionValue.raw = rawValIn;

	cxa_array_iterate(&adcChanIn->listeners, currListener, cxa_adcChannel_listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_convComp != NULL ) currListener->cb_convComp(adcChanIn, wasSuccessfulIn, voltageIn, rawValIn, currListener->userVar);
	}
}


// ******** local function implementations ********
