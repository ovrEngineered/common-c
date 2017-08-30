/**
 * @file
 *
 * @note This object should work across all architecture-specific implementations
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * @endcode
 *
 *
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
#ifndef CXA_TEMPSENSOR_H_
#define CXA_TEMPSENSOR_H_


// ******** includes ********
#include <stdbool.h>

#include <cxa_array.h>


// ******** global macro definitions ********
#define CXA_TEMPSENSE_CTOF(degCIn)			(((degCIn) * 1.8) + 32.0)


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_tempSensor cxa_tempSensor_t;


/**
 * @public
 */
typedef void (*cxa_tempSensor_cb_updatedValue_t)(cxa_tempSensor_t *const tmpSnsIn, bool wasSuccessfulIn, bool valueDidChangeIn, float newTemp_degCIn, void* userVarIn);


/**
 * @protected
 */
typedef bool (*cxa_tempSensor_scm_requestNewValue_t)(cxa_tempSensor_t *const superIn);


/**
 * @private
 */
struct cxa_tempSensor
{
	cxa_tempSensor_scm_requestNewValue_t scm_requestNewValue;

	float lastReading_degC;

	cxa_tempSensor_cb_updatedValue_t cb_onTempUpdate;
	void* userVar;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_tempSensor_init(cxa_tempSensor_t *const tmpSnsIn, cxa_tempSensor_scm_requestNewValue_t scm_requestNewValIn);


/**
 * @public
 */
bool cxa_tempSensor_getValue_withCallback(cxa_tempSensor_t *const tmpSnsIn, cxa_tempSensor_cb_updatedValue_t cbIn, void* userVarIn);


/**
 * @public
 */
float cxa_tempSensor_getLastValue_degC(cxa_tempSensor_t *const tmpSnsIn);


/**
 * @protected
 */
void cxa_tempSensor_notify_updatedValue(cxa_tempSensor_t *const tmpSnsIn, bool wasSuccessfulIn, float newTemp_degCIn);

#endif /* CXA_TEMPSENSOR_H_ */
