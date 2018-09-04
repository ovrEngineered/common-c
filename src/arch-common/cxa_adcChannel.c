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


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_adcChannel_init(cxa_adcChannel_t* const adcChanIn, cxa_adcChannel_scm_startConversion_singleShot_t scm_startConv_ssIn)
{
	cxa_assert(adcChanIn);
	cxa_assert(scm_startConv_ssIn);

	// save our references
	adcChanIn->scm_startConv_ss = scm_startConv_ssIn;

	// setup our listeners
	cxa_array_initStd(&adcChanIn->listeners, adcChanIn->listeners_raw);
}


void cxa_adcChannel_addListener(cxa_adcChannel_t *const adcChanIn,
						 cxa_adcChannel_cb_conversionComplete_t cb_convCompIn,
						 cxa_adcChannel_cb_conversionComplete_raw_t cb_convComp_rawIn,
						 void* userVarIn)
{
	cxa_assert(adcChanIn);
	cxa_assert( (cb_convCompIn != NULL) || (cb_convComp_rawIn != NULL) );

	cxa_adcChannel_listener_t newListener =
	{
			.cb_convComp = cb_convCompIn,
			.cb_convComp_raw = cb_convComp_rawIn,
			.userVar = userVarIn
	};

	cxa_assert(cxa_array_append(&adcChanIn->listeners, &newListener));
}


bool cxa_adcChannel_startConversion_singleShot(cxa_adcChannel_t *const adcChanIn)
{
	cxa_assert(adcChanIn);
	cxa_assert(adcChanIn->scm_startConv_ss);
	return adcChanIn->scm_startConv_ss(adcChanIn);
}


void cxa_adcChannel_notify_conversionComplete(cxa_adcChannel_t *const adcChanIn, float voltageIn, const uint8_t* rawValIn, size_t rawValLenIn)
{
	cxa_assert(adcChanIn);

	cxa_array_iterate(&adcChanIn->listeners, currListener, cxa_adcChannel_listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_convComp != NULL ) currListener->cb_convComp(adcChanIn, voltageIn, currListener->userVar);
		if( currListener->cb_convComp_raw != NULL ) currListener->cb_convComp_raw(adcChanIn, rawValIn, rawValLenIn, currListener->userVar);
	}
}


// ******** local function implementations ********
