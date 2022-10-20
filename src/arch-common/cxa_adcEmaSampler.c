/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_adcEmaSampler.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_runLoop.h>
#include <math.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void runLoopCb_onUpdate(void* userVarIn);
static void adcCb_onConversionComplete(cxa_adcChannel_t *const adcChanIn, bool wasSuccessfulIn, float readVoltageIn, uint16_t rawValueIn, void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_adcEmaSampler_init(cxa_adcEmaSampler_t *const adcSampIn, cxa_adcChannel_t *const adcChanIn, float smoothingConstantIn, uint32_t samplePeriod_msIn, int threadIdIn)
{
	cxa_assert(adcSampIn);
	cxa_assert(adcChanIn);

	// save our references
	adcSampIn->adcChan = adcChanIn;
	adcSampIn->smoothingConstant = smoothingConstantIn;
	adcSampIn->samplePeriod_sec = ((float)samplePeriod_msIn) / 1000.0;
	adcSampIn->currVal_v = NAN;
	adcSampIn->prevVal_v = NAN;

	// register for adc samples
	cxa_adcChannel_addListener(adcSampIn->adcChan, adcCb_onConversionComplete, (void*)adcSampIn);

	// start sampling
	cxa_runLoop_addTimedEntry(threadIdIn, samplePeriod_msIn, NULL, runLoopCb_onUpdate, (void*)adcSampIn);
}


void cxa_adcEmaSampler_resetAverage(cxa_adcEmaSampler_t *const adcSampIn)
{
	cxa_assert(adcSampIn);

	adcSampIn->currVal_v = NAN;
	adcSampIn->prevVal_v = NAN;
}


cxa_adcEmaSampler_retVal_t cxa_adcEmaSampler_getCurrentAverage(cxa_adcEmaSampler_t *const adcSampIn)
{
	cxa_assert(adcSampIn);

	cxa_adcEmaSampler_retVal_t retVal = {
											.currAverage_v = adcSampIn->currVal_v,
											.currDerivative_vps = (adcSampIn->currVal_v - adcSampIn->prevVal_v) / adcSampIn->samplePeriod_sec
										};

	return retVal;
}


// ******** local function implementations ********
static void runLoopCb_onUpdate(void* userVarIn)
{
	cxa_adcEmaSampler_t* adcSampIn = (cxa_adcEmaSampler_t*)userVarIn;
	cxa_assert(adcSampIn);

	// start a new sample
	cxa_adcChannel_startConversion_singleShot(adcSampIn->adcChan);
}


static void adcCb_onConversionComplete(cxa_adcChannel_t *const adcChanIn, bool wasSuccessfulIn, float readVoltageIn, uint16_t rawValueIn, void* userVarIn)
{
	cxa_adcEmaSampler_t* adcSampIn = (cxa_adcEmaSampler_t*)userVarIn;
	cxa_assert(adcSampIn);

	// got a new sample...make sure it's good
	if( !wasSuccessfulIn ) return;

	// calculate our new value
	adcSampIn->prevVal_v = adcSampIn->currVal_v;
	adcSampIn->currVal_v = isnan(adcSampIn->currVal_v) ?
						   readVoltageIn :
						   (adcSampIn->smoothingConstant * readVoltageIn) + ((1.0-adcSampIn->smoothingConstant) * adcSampIn->currVal_v);
}
