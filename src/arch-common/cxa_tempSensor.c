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
	tmpSnsIn->cb_onTempUpdate = NULL;
	tmpSnsIn->userVar = NULL;

	tmpSnsIn->lastReading_degC = NAN;
}


bool cxa_tempSensor_getValue_withCallback(cxa_tempSensor_t *const tmpSnsIn, cxa_tempSensor_cb_updatedValue_t cbIn, void* userVarIn)
{
	cxa_assert(tmpSnsIn);
	cxa_assert(tmpSnsIn->scm_requestNewValue);

	// make sure we don't have a read in progress
	if( tmpSnsIn->cb_onTempUpdate != NULL ) return false;

	// save our callback
	tmpSnsIn->cb_onTempUpdate = cbIn;
	tmpSnsIn->userVar = userVarIn;

	// try to start our read
	bool retVal = tmpSnsIn->scm_requestNewValue(tmpSnsIn);

	// cleanup if the request failed
	if( !retVal )
	{
		tmpSnsIn->cb_onTempUpdate = NULL;
		tmpSnsIn->userVar = NULL;
	}

	return retVal;
}


float cxa_tempSensor_getLastValue_degC(cxa_tempSensor_t *const tmpSnsIn)
{
	cxa_assert(tmpSnsIn);

	return tmpSnsIn->lastReading_degC;
}


void cxa_tempSensor_notify_updatedValue(cxa_tempSensor_t *const tmpSnsIn, bool wasSuccessfulIn, float newTemp_degCIn)
{
	cxa_assert(tmpSnsIn);

	tmpSnsIn->lastReading_degC = newTemp_degCIn;

	if( tmpSnsIn->cb_onTempUpdate != NULL ) tmpSnsIn->cb_onTempUpdate(tmpSnsIn, wasSuccessfulIn, newTemp_degCIn, tmpSnsIn->userVar);
	tmpSnsIn->cb_onTempUpdate = NULL;
	tmpSnsIn->userVar = NULL;
}


// ******** local function implementations ********
