/**
 * @copyright 2016 opencxa.org
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
#ifndef CXA_PCA9624_H_
#define CXA_PCA9624_H_


// ******** includes ********
#include <cxa_i2cMaster.h>
#include <cxa_led.h>


// ******** global macro definitions ********
#define CXA_PCA9624_NUM_CHANNELS			8


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_pca9624 cxa_pca9624_t;


/**
 * @public
 */
typedef struct
{
	uint8_t channelIndex;
	uint8_t brightness;
}cxa_pca9624_channelEntry_t;


/**
 * @private
 */
struct cxa_pca9624
{
	bool isInit;

	cxa_i2cMaster_t* i2c;
	uint8_t address;

	cxa_gpio_t* gpio_outputEnable;

	uint8_t currRegs[18];
};


// ******** global function prototypes ********
bool cxa_pca9624_init(cxa_pca9624_t *const pcaIn, cxa_i2cMaster_t *const i2cIn, uint8_t addressIn, cxa_gpio_t *const gpio_oeIn);

bool cxa_pca9624_setGlobalBlinkRate(cxa_pca9624_t *const pcaIn, uint16_t onPeriod_msIn, uint16_t offPeriod_msIn);

bool cxa_pca9624_setBrightnessForChannels(cxa_pca9624_t *const pcaIn, cxa_pca9624_channelEntry_t* chansEntriesIn, size_t numChansIn);
bool cxa_pca9624_blinkChannels(cxa_pca9624_t *const pcaIn, cxa_pca9624_channelEntry_t* chansEntriesIn, size_t numChansIn);

#endif // CXA_PCA9624_H_
