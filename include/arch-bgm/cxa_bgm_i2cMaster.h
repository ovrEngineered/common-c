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
#ifndef CXA_BGM_I2C_MASTER_H_
#define CXA_BGM_I2C_MASTER_H_


// ******** includes ********
#include <cxa_i2cMaster.h>

#include <em_gpio.h>
#include <em_i2c.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_bgm_i2cMaster_t object
 */
typedef struct cxa_bgm_i2cMaster cxa_bgm_i2cMaster_t;


/**
 * @private
 */
typedef struct
{
	GPIO_Port_TypeDef port;
	unsigned int pinNum;
	uint32_t loc;
}cxa_bgm_pinSetup_t;


/**
 * @private
 */
struct cxa_bgm_i2cMaster
{
	cxa_i2cMaster_t super;

	I2C_TypeDef* i2cPort;

	cxa_bgm_pinSetup_t sda;
	cxa_bgm_pinSetup_t scl;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_bgm_i2cMaster_init(cxa_bgm_i2cMaster_t *const i2cIn, I2C_TypeDef *const i2cPortIn,
							const GPIO_Port_TypeDef sdaPortNumIn, const unsigned int sdaPinNumIn, uint32_t sdaLocIn,
							const GPIO_Port_TypeDef sclPortNumIn, const unsigned int sclPinNumIn, uint32_t sclLocIn);


#endif // CXA_BGM_I2C_H_
