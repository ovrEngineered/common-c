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
#include "cxa_lightSensor.h"


// ******** includes ********
#include <math.h>

#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_lightSensor_init(cxa_lightSensor_t *const lightSnsIn, cxa_lightSensor_scm_requestNewValue_t scm_requestNewValIn)
{
	cxa_assert(lightSnsIn);
	cxa_assert(scm_requestNewValIn);

	// save our references
	lightSnsIn->scm_requestNewValue = scm_requestNewValIn;
	lightSnsIn->cb_onUpdate = NULL;
	lightSnsIn->userVar = NULL;

	lightSnsIn->lastReading_255 = 0;
	lightSnsIn->wasLastReadSuccessful = false;
}


bool cxa_lightSensor_getValue_withCallback(cxa_lightSensor_t *const lightSnsIn, cxa_lightSensor_cb_updatedValue_t cbIn, void* userVarIn)
{
	cxa_assert(lightSnsIn);
	cxa_assert(lightSnsIn->scm_requestNewValue);

	// make sure we don't have a read in progress
	if( lightSnsIn->cb_onUpdate != NULL ) return false;

	// save our callback
	lightSnsIn->cb_onUpdate = cbIn;
	lightSnsIn->userVar = userVarIn;

	// try to start our read
	bool retVal = lightSnsIn->scm_requestNewValue(lightSnsIn);

	// cleanup if the request failed
	if( !retVal )
	{
		lightSnsIn->cb_onUpdate = NULL;
		lightSnsIn->userVar = NULL;
	}

	return retVal;
}


bool cxa_lightSensor_wasLastReadSuccessful(cxa_lightSensor_t *const lightSnsIn)
{
	cxa_assert(lightSnsIn);

	return lightSnsIn->wasLastReadSuccessful;
}


uint8_t cxa_lightSensor_getLastValue_255(cxa_lightSensor_t *const lightSnsIn)
{
	cxa_assert(lightSnsIn);

	return lightSnsIn->lastReading_255;
}


void cxa_lightSensor_notify_updatedValue(cxa_lightSensor_t *const lightSnsIn, bool wasSuccessfulIn, uint8_t newLight_255In)
{
	cxa_assert(lightSnsIn);

	bool valueDidChange = (lightSnsIn->lastReading_255 != newLight_255In);
	lightSnsIn->wasLastReadSuccessful = wasSuccessfulIn;
	lightSnsIn->lastReading_255 = newLight_255In;

	if( lightSnsIn->cb_onUpdate != NULL ) lightSnsIn->cb_onUpdate(lightSnsIn, wasSuccessfulIn, valueDidChange, newLight_255In, lightSnsIn->userVar);
	lightSnsIn->cb_onUpdate = NULL;
	lightSnsIn->userVar = NULL;
}


// ******** local function implementations ********
