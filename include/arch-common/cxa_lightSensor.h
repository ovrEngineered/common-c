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
#ifndef CXA_LIGHTSENSOR_H_
#define CXA_LIGHTSENSOR_H_


// ******** includes ********
#include <stdbool.h>
#include <stdint.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_lightSensor cxa_lightSensor_t;


/**
 * @public
 */
typedef void (*cxa_lightSensor_cb_updatedValue_t)(cxa_lightSensor_t *const lightSnsIn, bool wasSuccessfulIn, uint8_t newLight_255In, void* userVarIn);


/**
 * @protected
 */
typedef bool (*cxa_lightSensor_scm_requestNewValue_t)(cxa_lightSensor_t *const superIn);


/**
 * @private
 */
struct cxa_lightSensor
{
	cxa_lightSensor_scm_requestNewValue_t scm_requestNewValue;

	uint8_t lastReading_255;

	cxa_lightSensor_cb_updatedValue_t cb_onUpdate;
	void* userVar;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_lightSensor_init(cxa_lightSensor_t *const lightSnsIn, cxa_lightSensor_scm_requestNewValue_t scm_requestNewValIn);


/**
 * @public
 */
bool cxa_lightSensor_getValue_withCallback(cxa_lightSensor_t *const lightSnsIn, cxa_lightSensor_cb_updatedValue_t cbIn, void* userVarIn);


/**
 * @public
 */
uint8_t cxa_lightSensor_getLastValue_255(cxa_lightSensor_t *const lightSnsIn);


/**
 * @protected
 */
void cxa_lightSensor_notify_updatedValue(cxa_lightSensor_t *const lightSnsIn, bool wasSuccessfulIn, uint8_t newLight_255In);


#endif /* CXA_LIGHTSENSOR_H_ */
