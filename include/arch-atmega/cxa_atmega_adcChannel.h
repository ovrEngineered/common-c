/**
 * @file
 *
 * #### Example Usage: ####
 *
 * @code
 * @endcode
 *
 *
 * @copyright 2018 opencxa.org
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
#ifndef CXA_ATMEGA_ADCCHANNEL_H_
#define CXA_ATMEGA_ADCCHANNEL_H_


// ******** includes ********
#include <cxa_adcChannel.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_atmega_adcChannel cxa_atmega_adcChannel_t;


/**
 * @public
 */
typedef enum
{
	CXA_ATM_ADCCHAN_0 = 0,
	CXA_ATM_ADCCHAN_1 = 1,
	CXA_ATM_ADCCHAN_2 = 2,
	CXA_ATM_ADCCHAN_3 = 3,
	CXA_ATM_ADCCHAN_4 = 4,
	CXA_ATM_ADCCHAN_5 = 5,
	CXA_ATM_ADCCHAN_6 = 6,
	CXA_ATM_ADCCHAN_7 = 7
}cxa_atmega_adcChannel_id_t;


typedef enum
{
	CXA_ATM_ADCCHAN_REF_AREF = 0,
	CXA_ATM_ADCCHAN_REF_AVCC = 1,
	CXA_ATM_ADCCHAN_REF_INTERNAL = 3
}cxa_atmega_adcChannel_reference_t;


/**
 * @private
 */
struct cxa_atmega_adcChannel
{
	cxa_adcChannel_t super;

	cxa_atmega_adcChannel_id_t chanId;
	cxa_atmega_adcChannel_reference_t reference;
};


// ******** global function prototypes ********
/**
 * @public
 */
void cxa_atmega_adcChannel_init(cxa_atmega_adcChannel_t* const adcChanIn, const cxa_atmega_adcChannel_id_t chanIdIn,
								const cxa_atmega_adcChannel_reference_t referenceIn);


#endif /* CXA_ATMEGA_ADCCHANNEL_H_ */
