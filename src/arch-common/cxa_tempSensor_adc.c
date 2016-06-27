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
#include "cxa_tempSensor_adc.h"


// ******** includes ********
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static bool scm_requestNewValue(cxa_tempSensor_t *const tempSnsIn);
static void scm_update(cxa_tempSensor_t *const tempSnsIn);

static void cb_adcConvComplete(cxa_adcChannel_t *const adcChanIn, float readVoltageIn, void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_tempSensor_adc_init_onePoint(cxa_tempSensor_adc_t *const tempSnsIn, cxa_adcChannel_t* adcChanIn,
							 float knownTemp_cIn, float vAtKnownTempIn)
{
	cxa_assert(tempSnsIn);
	cxa_assert(adcChanIn);

	// save our references
	tempSnsIn->adc = adcChanIn;
	tempSnsIn->onePointCal.knownTemp_c = knownTemp_cIn;
	tempSnsIn->onePointCal.vAtKnowTemp = vAtKnownTempIn;

	// register our ADC listener
	cxa_adcChannel_addListener(tempSnsIn->adc, cb_adcConvComplete, NULL, (void*)tempSnsIn);

	// initialize our super class
	cxa_tempSensor_init(&tempSnsIn->super, scm_requestNewValue, scm_update);
}


// ******** local function implementations ********
static bool scm_requestNewValue(cxa_tempSensor_t *const superIn)
{
	cxa_tempSensor_adc_t* tempSnsIn = (cxa_tempSensor_adc_t*)superIn;
	cxa_assert(tempSnsIn);

	return cxa_adcChannel_startConversion_singleShot(tempSnsIn->adc);
}


static void scm_update(cxa_tempSensor_t *const superIn)
{
	cxa_tempSensor_adc_t* tempSnsIn = (cxa_tempSensor_adc_t*)superIn;
	cxa_assert(tempSnsIn);

	cxa_adcChannel_update(tempSnsIn->adc);
}


static void cb_adcConvComplete(cxa_adcChannel_t *const adcChanIn, float readVoltageIn, void* userVarIn)
{
	cxa_tempSensor_adc_t* tempSnsIn = (cxa_tempSensor_adc_t*)userVarIn;
	cxa_assert(tempSnsIn);

	// calculate our new temp
	float temp_c = (readVoltageIn * tempSnsIn->onePointCal.knownTemp_c) / tempSnsIn->onePointCal.vAtKnowTemp;

	// call our callback
	if( tempSnsIn->super.cb_onTempUpdate != NULL ) tempSnsIn->super.cb_onTempUpdate(&tempSnsIn->super, temp_c, tempSnsIn->super.userVar);

	// cleanup
	tempSnsIn->super.cb_onTempUpdate = NULL;
	tempSnsIn->super.userVar = NULL;
}
