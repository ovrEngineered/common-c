/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_batteryCapacityEstimator.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_numberUtils.h>
#include <math.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void cb_adcConvComplete(cxa_adcChannel_t *const adcChanIn, bool wasSuccessfulIn, float readVoltageIn, uint16_t rawValueIn, void* userVarIn);


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

	// start our conversion
	cxa_adcChannel_startConversion_singleShot(bceIn->adc_battVoltage);

	return true;
}


// ******** local function implementations ********
static void cb_adcConvComplete(cxa_adcChannel_t *const adcChanIn, bool wasSuccessfulIn, float readVoltageIn, uint16_t rawValueIn, void* userVarIn)
{
	cxa_batteryCapacityEstimator_t* bceIn = (cxa_batteryCapacityEstimator_t*)userVarIn;
	cxa_assert(bceIn);

	if( !wasSuccessfulIn )
	{
		if( bceIn->cb_updateValue != NULL ) bceIn->cb_updateValue(bceIn, false, NAN, bceIn->userVar);
		return;
	}

	// adjust our read voltage
	readVoltageIn *= bceIn->battVoltMult;

	// calculate our percentage
	float battPcnt = CXA_MAX((readVoltageIn - bceIn->minVoltage), 0.0) / (bceIn->maxVoltage - bceIn->minVoltage);
	battPcnt = CXA_MIN(battPcnt, 1.0);

	// call our callback
	if( bceIn->cb_updateValue != NULL ) bceIn->cb_updateValue(bceIn, true, battPcnt, bceIn->userVar);

	// cleanup
	bceIn->cb_updateValue = NULL;
	bceIn->userVar = NULL;
}
