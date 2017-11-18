/**
 * @file
 * @copyright 2017 opencxa.org
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
#ifndef CXA_LIGHTSENSOR_LTR329_H_
#define CXA_LIGHTSENSOR_LTR329_H_


// ******** includes ********
#include <cxa_i2cMaster.h>
#include <cxa_lightSensor.h>
#include <cxa_stateMachine.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct
{
	cxa_lightSensor_t super;

	cxa_i2cMaster_t* i2c;

	uint16_t readVal;

	cxa_stateMachine_t stateMachine;
}cxa_lightSensor_ltr329_t;


// ******** global function prototypes ********
void cxa_lightSensor_ltr329_init(cxa_lightSensor_ltr329_t *const lightSnsIn, cxa_i2cMaster_t *const i2cIn, int threadIdIn);


#endif
