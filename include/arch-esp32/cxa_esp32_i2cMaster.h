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
#ifndef CXA_ESP32_I2C_MASTER_H_
#define CXA_ESP32_I2C_MASTER_H_


// ******** includes ********
#include <cxa_i2cMaster.h>

#include "driver/i2c.h"


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_esp32_i2cMaster_t object
 */
typedef struct cxa_esp32_i2cMaster cxa_esp32_i2cMaster_t;


/**
 * @private
 */
struct cxa_esp32_i2cMaster
{
	cxa_i2cMaster_t super;

	i2c_port_t i2cPort;

	i2c_config_t conf;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_esp32_i2cMaster_init(cxa_esp32_i2cMaster_t *const i2cIn, const i2c_port_t i2cPortIn,
							 const gpio_num_t pinNum_sdaIn, const gpio_num_t pinNum_sclIn,
							 const bool enablePullUpsIn, uint32_t busFreq_hzIn);


#endif // CXA_BLUEGIGA_I2C_H_
