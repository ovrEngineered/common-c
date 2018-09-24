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
#include <math.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define KELVIN_OFFSET			273.15


// ******** local type definitions ********


// ******** local function prototypes ********
static bool scm_requestNewValue(cxa_tempSensor_t *const tempSnsIn);

static void cb_adcConvComplete(cxa_adcChannel_t *const adcChanIn, float readVoltageIn, uint16_t rawValueIn, void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_tempSensor_adc_init_voltageOnePoint(cxa_tempSensor_adc_t *const tempSnsIn, cxa_adcChannel_t* adcChanIn,
							 	 	 	 	 float knownTemp_cIn, float vAtKnownTempIn)
{
	cxa_assert(tempSnsIn);
	cxa_assert(adcChanIn);

	// save our references
	tempSnsIn->adc = adcChanIn;
	tempSnsIn->calibrationVals.onePointCal.knownTemp_c = knownTemp_cIn;
	tempSnsIn->calibrationVals.onePointCal.vAtKnownTemp = vAtKnownTempIn;
	tempSnsIn->calibrationType = CXA_TEMPSENSOR_CALTYPE_ONEPOINTVOLTAGE;

	// register our ADC listener
	cxa_adcChannel_addListener(tempSnsIn->adc, cb_adcConvComplete, (void*)tempSnsIn);

	// initialize our super class
	cxa_tempSensor_init(&tempSnsIn->super, scm_requestNewValue);
}


void cxa_tempSensor_adc_init_beta(cxa_tempSensor_adc_t *const tempSnsIn, cxa_adcChannel_t *const adcChanIn,
								  float r1_ohmIn, float r0_ohmIn, float t0_cIn, float betaIn)
{
	cxa_assert(tempSnsIn);
	cxa_assert(adcChanIn);

	// save our references
	tempSnsIn->adc = adcChanIn;
	tempSnsIn->calibrationVals.beta.r1_ohm = r1_ohmIn;
	tempSnsIn->calibrationVals.beta.r0_ohm = r0_ohmIn;
	tempSnsIn->calibrationVals.beta.t0_c = t0_cIn;
	tempSnsIn->calibrationVals.beta.beta = betaIn;
	tempSnsIn->calibrationType = CXA_TEMPSENSOR_CALTYPE_BETA;

	// register our ADC listener
	cxa_adcChannel_addListener(tempSnsIn->adc, cb_adcConvComplete, (void*)tempSnsIn);

	// initialize our super class
	cxa_tempSensor_init(&tempSnsIn->super, scm_requestNewValue);
}


// ******** local function implementations ********
static bool scm_requestNewValue(cxa_tempSensor_t *const superIn)
{
	cxa_tempSensor_adc_t* tempSnsIn = (cxa_tempSensor_adc_t*)superIn;
	cxa_assert(tempSnsIn);

	return cxa_adcChannel_startConversion_singleShot(tempSnsIn->adc);
}


static void cb_adcConvComplete(cxa_adcChannel_t *const adcChanIn, float readVoltageIn, uint16_t rawValueIn, void* userVarIn)
{
	cxa_tempSensor_adc_t* tempSnsIn = (cxa_tempSensor_adc_t*)userVarIn;
	cxa_assert(tempSnsIn);

	switch( tempSnsIn->calibrationType )
	{
		case CXA_TEMPSENSOR_CALTYPE_ONEPOINTVOLTAGE:
		{
			float temp_c = (readVoltageIn * tempSnsIn->calibrationVals.onePointCal.knownTemp_c) / tempSnsIn->calibrationVals.onePointCal.vAtKnownTemp;
			cxa_tempSensor_notify_updatedValue(&tempSnsIn->super, true, temp_c);
			break;
		}

		case CXA_TEMPSENSOR_CALTYPE_BETA:
		{
			float temp_c = NAN;

			uint16_t maxRawValue = cxa_adcChannel_getMaxRawValue(tempSnsIn->adc);
			if( (rawValueIn > 0) && (rawValueIn != maxRawValue) )
			{
				float r_therm = -((float)rawValueIn * tempSnsIn->calibrationVals.beta.r1_ohm) / ((float)rawValueIn - (float)maxRawValue);
				temp_c = (tempSnsIn->calibrationVals.beta.beta * (tempSnsIn->calibrationVals.beta.t0_c + KELVIN_OFFSET)) /
						 (tempSnsIn->calibrationVals.beta.beta + ((tempSnsIn->calibrationVals.beta.t0_c + KELVIN_OFFSET) * log10(r_therm / tempSnsIn->calibrationVals.beta.r0_ohm))) -
						 KELVIN_OFFSET;
			}

			cxa_tempSensor_notify_updatedValue(&tempSnsIn->super, true, temp_c);
			break;
		}
	}
}
