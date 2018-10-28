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
#ifndef CXA_BATTERYCAPACITYESTIMATOR_H_
#define CXA_BATTERYCAPACITYESTIMATOR_H_


// ******** includes ********
#include <cxa_adcChannel.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_batteryCapacityEstimator cxa_batteryCapacityEstimator_t;


/**
 * @public
 */
typedef void (*cxa_batteryCapacityEstimator_cb_updatedValue_t)(cxa_batteryCapacityEstimator_t *const cbeIn, bool wasSuccessfulIn, float battPcntIn, void* userVarIn);


/**
 * @private
 */
struct cxa_batteryCapacityEstimator
{
	cxa_adcChannel_t* adc_battVoltage;

	float battVoltMult;
	float maxVoltage;
	float minVoltage;

	cxa_batteryCapacityEstimator_cb_updatedValue_t cb_updateValue;
	void* userVar;
};


// ******** global function prototypes ********
void cxa_batteryCapacityEstimator_init(cxa_batteryCapacityEstimator_t *const bceIn, cxa_adcChannel_t* adcIn, float battVoltMultIn, float maxVoltageIn, float minVoltageIn);

bool cxa_batteryCapacityEstimator_getValue_withCallback(cxa_batteryCapacityEstimator_t *const bceIn, cxa_batteryCapacityEstimator_cb_updatedValue_t cbIn, void* userVarIn);


#endif /* CXA_BATTERYCAPACITYESTIMATOR_H_ */
