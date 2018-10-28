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
#include "cxa_tempSensor.h"


// ******** includes ********
#include <math.h>

#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_tempSensor_init(cxa_tempSensor_t *const tmpSnsIn, cxa_tempSensor_scm_requestNewValue_t scm_requestNewValIn)
{
	cxa_assert(tmpSnsIn);
	cxa_assert(scm_requestNewValIn);

	// save our references
	tmpSnsIn->scm_requestNewValue = scm_requestNewValIn;

	tmpSnsIn->wasLastReadSuccessful = false;
	tmpSnsIn->lastReading_degC = NAN;

	cxa_array_initStd(&tmpSnsIn->listeners, tmpSnsIn->listeners_raw);
}


void cxa_tempSensor_addListener(cxa_tempSensor_t *const tmpSnsIn, cxa_tempSensor_cb_updatedValue_t cbIn, void* userVarIn)
{
	cxa_assert(tmpSnsIn);
	cxa_assert(cbIn);

	cxa_tempSensor_listenerEntry_t newEntry = {.cb_onTempUpdate = cbIn, .userVar = userVarIn};
	cxa_assert_msg(cxa_array_append(&tmpSnsIn->listeners, &newEntry), "increase CXA_TEMPSENSE_MAXNUM_LISTENERS");
}


void cxa_tempSensor_requestNewValueNow(cxa_tempSensor_t *const tmpSnsIn)
{
	cxa_assert(tmpSnsIn);
	cxa_assert(tmpSnsIn->scm_requestNewValue);

	// start our read
	tmpSnsIn->scm_requestNewValue(tmpSnsIn);
}


bool cxa_tempSensor_wasLastReadSuccessful(cxa_tempSensor_t *const tmpSnsIn)
{
	cxa_assert(tmpSnsIn);

	return tmpSnsIn->wasLastReadSuccessful;
}


float cxa_tempSensor_getLastValue_degC(cxa_tempSensor_t *const tmpSnsIn)
{
	cxa_assert(tmpSnsIn);

	return tmpSnsIn->lastReading_degC;
}


void cxa_tempSensor_notify_updatedValue(cxa_tempSensor_t *const tmpSnsIn, bool wasSuccessfulIn, float newTemp_degCIn)
{
	cxa_assert(tmpSnsIn);

	bool valueDidChange = (tmpSnsIn->lastReading_degC != newTemp_degCIn);
	tmpSnsIn->wasLastReadSuccessful = wasSuccessfulIn;
	tmpSnsIn->lastReading_degC = newTemp_degCIn;

	cxa_array_iterate(&tmpSnsIn->listeners, currListener, cxa_tempSensor_listenerEntry_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_onTempUpdate != NULL ) currListener->cb_onTempUpdate(tmpSnsIn, wasSuccessfulIn, valueDidChange, newTemp_degCIn, currListener->userVar);
	}
}


// ******** local function implementations ********
