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


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_pca9624 cxa_pca9624_t;


/**
 * @private
 */
typedef struct
{
	cxa_led_t super;

	cxa_pca9624_t* pca;
	uint8_t chanNum;
}cxa_led_pca9624_t;


/**
 * @private
 */
struct cxa_pca9624
{
	cxa_i2cMaster_t* i2c;
	uint8_t address;

	cxa_gpio_t* gpio_outputEnable;

	cxa_led_pca9624_t leds[8];
};


// ******** global function prototypes ********
bool cxa_pca9624_init(cxa_pca9624_t *const pcaIn, cxa_i2cMaster_t *const i2cIn, uint8_t addressIn, cxa_gpio_t *const gpio_oeIn);

cxa_led_t* cxa_pca9624_getLed(cxa_pca9624_t *const pcaIn, uint8_t chanNumIn);

#endif // CXA_PCA9624_H_
