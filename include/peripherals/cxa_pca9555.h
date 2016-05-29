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
#ifndef CXA_PCA9555_H_
#define CXA_PCA9555_H_


// ******** includes ********
#include <cxa_i2cMaster.h>
#include <cxa_gpio.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_pca9555 cxa_pca9555_t;


/**
 * @private
 */
typedef struct
{
	cxa_gpio_t super;

	cxa_pca9555_t* pca;
	uint8_t chanNum;

	cxa_gpio_polarity_t polarity;
	cxa_gpio_direction_t lastDirection;
	bool lastOutputVal;
}cxa_gpio_pca9555_t;


/**
 * @private
 */
struct cxa_pca9555
{
	cxa_i2cMaster_t* i2c;
	uint8_t address;

	cxa_gpio_pca9555_t gpios_port0[8];
	cxa_gpio_pca9555_t gpios_port1[8];
};


// ******** global function prototypes ********
bool cxa_pca9555_init(cxa_pca9555_t *const pcaIn, cxa_i2cMaster_t *const i2cIn, uint8_t addressIn);

cxa_gpio_t* cxa_pca9555_getGpio(cxa_pca9555_t *const pcaIn, uint8_t portNumIn, uint8_t chanNumIn);

#endif // CXA_PCA9555_H_
