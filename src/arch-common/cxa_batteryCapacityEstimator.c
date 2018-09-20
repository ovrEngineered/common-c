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
#include "cxa_batteryCapacityEstimator.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_numberUtils.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void cb_adcConvComplete(cxa_adcChannel_t *const adcChanIn, float readVoltageIn, uint16_t rawValueIn, void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_batteryCapacityEstimator_init(cxa_batteryCapacityEstimator_t *const bceIn, cxa_adcChannel_t* adcIn, float battVoltMultIn, float maxVoltageIn, float minVoltageIn)
{
	cxa_assert(bceIn);

	// save our references
	bceIn->adc_battVoltage = adcIn;
	bceIn->battVoltMult = battVoltMultIn;
	bceIn->maxVoltage = maxVoltageIn;
	bceIn->minVoltage = minVoltageIn;
	bceIn->cb_updateValue = NULL;
	bceIn->userVar = NULL;

	// register as a listener on our ADC
	cxa_adcChannel_addListener(bceIn->adc_battVoltage, cb_adcConvComplete, (void*)bceIn);
}


bool cxa_batteryCapacityEstimator_getValue_withCallback(cxa_batteryCapacityEstimator_t *const bceIn, cxa_batteryCapacityEstimator_cb_updatedValue_t cbIn, void* userVarIn)
{
	cxa_assert(bceIn);
	cxa_assert(cbIn);

	// make sure we don't have a read in progress
	if( bceIn->cb_updateValue != NULL ) return false;

	// save our callback
	bceIn->cb_updateValue = cbIn;
	bceIn->userVar = userVarIn;

	// try to start our conversion
	bool retVal = cxa_adcChannel_startConversion_singleShot(bceIn->adc_battVoltage);

	// cleanup if we failed to start
	if( !retVal )
	{
		bceIn->cb_updateValue = NULL;
		bceIn->userVar = NULL;
	}
	return retVal;
}


// ******** local function implementations ********
static void cb_adcConvComplete(cxa_adcChannel_t *const adcChanIn, float readVoltageIn, uint16_t rawValueIn, void* userVarIn)
{
	cxa_batteryCapacityEstimator_t* bceIn = (cxa_batteryCapacityEstimator_t*)userVarIn;
	cxa_assert(bceIn);

	// adjust our read voltage
	readVoltageIn *= bceIn->battVoltMult;

	// calculate our percentage
	float battPcnt = CXA_MAX((readVoltageIn - bceIn->minVoltage), 0.0) / (bceIn->maxVoltage - bceIn->minVoltage);
	battPcnt = CXA_MIN(battPcnt, 1.0);

	// call our callback
	if( bceIn->cb_updateValue != NULL ) bceIn->cb_updateValue(bceIn, battPcnt, bceIn->userVar);

	// cleanup
	bceIn->cb_updateValue = NULL;
	bceIn->userVar = NULL;
}
