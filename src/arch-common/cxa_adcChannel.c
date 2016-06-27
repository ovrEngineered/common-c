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
void cxa_adcChannel_init(cxa_adcChannel_t* const adcChanIn, cxa_adcChannel_scm_startConversion_singleShot_t scm_startConv_ssIn, cxa_adcChannel_scm_update_t scm_updateIn)
{
	cxa_assert(adcChanIn);
	cxa_assert(scm_startConv_ssIn);

	// save our references
	adcChanIn->scm_startConv_ss = scm_startConv_ssIn;
	adcChanIn->scm_update = scm_updateIn;

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


void cxa_adcChannel_update(cxa_adcChannel_t *const adcChanIn)
{
	cxa_assert(adcChanIn);
	// this one isn't required
	if( adcChanIn->scm_update != NULL ) adcChanIn->scm_update(adcChanIn);
}


// ******** local function implementations ********
