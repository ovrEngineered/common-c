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
#include "cxa_ble112_adcChannel.h"


// ******** includes ********
#include <blestack/hw_regs.h>
#include <cxa_assert.h>
#include <cxa_runLoop.h>


#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define DECIMATION_RATE			0x03
#define INTERAL_VREF			1.24


// ******** local type definitions ********


// ******** local function prototypes ********
static bool scm_startConversion_singleShot(cxa_adcChannel_t *const superIn);

static void cb_onRunLoopUpdate(void* userVarIn);


// ********  local variable declarations *********
static cxa_ble112_adcChannel_t *currChan = NULL;


// ******** global function implementations ********
void cxa_ble112_adcChannel_init_internalRef(cxa_ble112_adcChannel_t *const adcChanIn, cxa_ble112_adcChannel_chan_t chanIn)
{
	cxa_assert(adcChanIn);

	// save our references
	adcChanIn->chan = chanIn;
	adcChanIn->vRef = CXA_BLE112_ADC_VREF_INTERNAL;

	// initialize our superclass
	cxa_adcChannel_init(&adcChanIn->super, scm_startConversion_singleShot);

	// set our pin to ADC mode
	APCFG |= (1 << chanIn);

	// register for run loop execution
	cxa_runLoop_addEntry(cb_onRunLoopUpdate, (void*)adcChanIn);
}


// ******** local function implementations ********
static bool scm_startConversion_singleShot(cxa_adcChannel_t *const superIn)
{
	cxa_ble112_adcChannel_t *adcChanIn = (cxa_ble112_adcChannel_t*)superIn;
	cxa_assert(adcChanIn);
	cxa_assert(adcChanIn->vRef == CXA_BLE112_ADC_VREF_INTERNAL);

	// make sure we aren't in the middle of a conversion
	if( currChan != NULL ) return false;

	// if this is a temperature sensor, we have some configuration to do
	// (do it here because we'll disable after to save power)
	if( adcChanIn->chan == CXA_BLE112_ADC_CHAN_INTTEMP )
	{
		TR0  |= (1 << 0);
		ATEST |= (1 << 0);
	}

	// start the conversion
	currChan = adcChanIn;
	ADCCON3 = (((uint8_t)adcChanIn->vRef) << 7) | (((uint8_t)DECIMATION_RATE) << 4) | ((uint8_t)adcChanIn->chan);
    return true;
}


static void cb_onRunLoopUpdate(void* userVarIn)
{
	cxa_ble112_adcChannel_t *adcChanIn = (cxa_ble112_adcChannel_t*)userVarIn;
	cxa_assert(adcChanIn);

	// see if we have anything to do
	if( currChan != adcChanIn ) return;

	// we're doing a conversion...check to see if it has completed
	if( !(ADCCON1 & (1 << 7)) ) return;

	// if this is a temperature sensor, disable the sensor to save power
	if( adcChanIn->chan == CXA_BLE112_ADC_CHAN_INTTEMP )
	{
		ATEST &= ~(1 << 0);
		TR0 &= ~(1 << 0);
	}

	// if we made it here, we _were_ doing a conversion, but it completed
	uint16_t adcResult = ((((((uint16_t)ADCH) << 8) | ((uint16_t)ADCL)) >> 2) & 0x0FFF);
	currChan = NULL;

	// let our listeners know
	cxa_array_iterate(&adcChanIn->super.listeners, currListener, cxa_adcChannel_listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_convComp != NULL ) currListener->cb_convComp(&adcChanIn->super, (float)adcResult/((float)(4096-1)) * INTERAL_VREF, currListener->userVar);
		if( currListener->cb_convComp_raw != NULL ) currListener->cb_convComp_raw(&adcChanIn->super, (uint8_t*)&adcResult, sizeof(adcResult), currListener->userVar);
	}
}
