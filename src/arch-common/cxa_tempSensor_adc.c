/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
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
#define DISCARD_TOPBOTTOM_RANGE_PCNT100					2.0


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_requestNewValue(cxa_tempSensor_t *const tempSnsIn);

static void cb_adcConvComplete(cxa_adcChannel_t *const adcChanIn, bool wasSuccessfulIn, float readVoltageIn, uint16_t rawValueIn, void* userVarIn);


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
static void scm_requestNewValue(cxa_tempSensor_t *const superIn)
{
	cxa_tempSensor_adc_t* tempSnsIn = (cxa_tempSensor_adc_t*)superIn;
	cxa_assert(tempSnsIn);

	// start the conversion
	cxa_adcChannel_startConversion_singleShot(tempSnsIn->adc);
}


static void cb_adcConvComplete(cxa_adcChannel_t *const adcChanIn, bool wasSuccessfulIn, float readVoltageIn, uint16_t rawValueIn, void* userVarIn)
{
	cxa_tempSensor_adc_t* tempSnsIn = (cxa_tempSensor_adc_t*)userVarIn;
	cxa_assert(tempSnsIn);

	if( !wasSuccessfulIn )
	{
		cxa_tempSensor_notify_updatedValue(&tempSnsIn->super, false, NAN);
		return;
	}
	// if we made it here, the conversion was successful

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
			if( ((maxRawValue * DISCARD_TOPBOTTOM_RANGE_PCNT100 / 100.0) < rawValueIn) && (rawValueIn < (maxRawValue * (100 - DISCARD_TOPBOTTOM_RANGE_PCNT100) / 100.0)) )
			{
				float r_therm = -((float)rawValueIn * tempSnsIn->calibrationVals.beta.r1_ohm) / ((float)rawValueIn - (float)maxRawValue);
				temp_c = (tempSnsIn->calibrationVals.beta.beta * (tempSnsIn->calibrationVals.beta.t0_c + CXA_CELSIUS_TO_KELVIN_OFFSET)) /
						 (tempSnsIn->calibrationVals.beta.beta + ((tempSnsIn->calibrationVals.beta.t0_c + CXA_CELSIUS_TO_KELVIN_OFFSET) * log(r_therm / tempSnsIn->calibrationVals.beta.r0_ohm))) -
						 CXA_CELSIUS_TO_KELVIN_OFFSET;
			}

			cxa_tempSensor_notify_updatedValue(&tempSnsIn->super, true, temp_c);
			break;
		}
	}
}
