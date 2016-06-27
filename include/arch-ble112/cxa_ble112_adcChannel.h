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
#ifndef CXA_BLE112_ADCCHAN_H_
#define CXA_BLE112_ADCCHAN_H_


// ******** includes ********
#include <cxa_adcChannel.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef enum
{
	CXA_BLE112_ADC_CHAN_AIN0 = 0x00,
	CXA_BLE112_ADC_CHAN_AIN1 = 0x01,
	CXA_BLE112_ADC_CHAN_AIN2 = 0x02,
	CXA_BLE112_ADC_CHAN_AIN3 = 0x03,
	CXA_BLE112_ADC_CHAN_AIN4 = 0x04,
	CXA_BLE112_ADC_CHAN_AIN5 = 0x05,
	CXA_BLE112_ADC_CHAN_AIN6 = 0x06,
	CXA_BLE112_ADC_CHAN_AIN7 = 0x07,
	CXA_BLE112_ADC_CHAN_INTTEMP = 0x0E,
	CXA_BLE112_ADC_CHAN_AVDD_DIV3 = 0x0F
}cxa_ble112_adcChannel_chan_t;


typedef enum
{
	CXA_BLE112_ADC_VREF_INTERNAL = 0x00,
	CXA_BLE112_ADC_VREF_EXTERNAL = 0x01,
	CXA_BLE112_ADC_VREF_AVDD5 = 0x02,
	CXA_BLE112_ADC_VREF_EXTERNAL_DIFF = 0x03
}cxa_ble112_adcChan_vref_t;


typedef struct
{
	cxa_adcChannel_t super;

	cxa_ble112_adcChannel_chan_t chan;
	cxa_ble112_adcChan_vref_t vRef;
}cxa_ble112_adcChannel_t;


// ******** global function prototypes ********
void cxa_ble112_adcChannel_init_internalRef(cxa_ble112_adcChannel_t *const adcChanIn, cxa_ble112_adcChannel_chan_t chanIn);


#endif /* CXA_BLE112_ADCCHAN_H_ */
