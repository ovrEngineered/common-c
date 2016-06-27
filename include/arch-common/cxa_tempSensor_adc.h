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
#ifndef CXA_TEMPSENSOR_ADC_H_
#define CXA_TEMPSENSOR_ADC_H_


// ******** includes ********
#include <cxa_adcChannel.h>
#include <cxa_tempSensor.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct
{
	cxa_tempSensor_t super;

	cxa_adcChannel_t* adc;

	struct
	{
		float knownTemp_c;
		float vAtKnowTemp;
	} onePointCal;

}cxa_tempSensor_adc_t;



// ******** global function prototypes ********
void cxa_tempSensor_adc_init_onePoint(cxa_tempSensor_adc_t *const tempSnsIn, cxa_adcChannel_t* adcChanIn,
							 float knownTemp_cIn, float vAtKnownTempIn);


#endif /* CXA_TEMPSENSOR_ADC_H_ */
