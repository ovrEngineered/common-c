/**
 * @file
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
#ifndef CXA_AWCU300_I2C_MASTER_H_
#define CXA_AWCU300_I2C_MASTER_H_


// ******** includes ********
#include <cxa_i2cMaster.h>
#include <lowlevel_drivers.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_awcu300_i2cMaster_t object
 */
typedef struct cxa_awcu300_i2cMaster cxa_awcu300_i2cMaster_t;


/**
 * @private
 */
struct cxa_awcu300_i2cMaster
{
	cxa_i2cMaster_t super;
	
	I2C_ID_Type i2cId;
};


// ******** global function prototypes ********
void cxa_awcu300_i2cMaster_init(cxa_awcu300_i2cMaster_t *const i2cIn, I2C_ID_Type i2cIdIn);


#endif // CXA_AWCU300_I2C_H_
