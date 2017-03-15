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
#ifndef CXA_BLUEGIGA_I2C_MASTER_H_
#define CXA_BLUEGIGA_I2C_MASTER_H_


// ******** includes ********
#include <cxa_i2cMaster.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_blueGiga_i2cMaster_t object
 */
typedef struct cxa_blueGiga_i2cMaster cxa_blueGiga_i2cMaster_t;


/**
 * @public
 * Forward declaration of blueGiga_btle_client to avoid
 * circular reference
 */
typedef struct cxa_blueGiga_btle_client cxa_blueGiga_btle_client_t;


/**
 * @private
 */
struct cxa_blueGiga_i2cMaster
{
	cxa_i2cMaster_t super;
	
	cxa_blueGiga_btle_client_t* btlec;

	size_t expectedNumBytesToWrite;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_blueGiga_i2cMaster_init(cxa_blueGiga_i2cMaster_t *const i2cIn, cxa_blueGiga_btle_client_t* btlecIn);


#endif // CXA_BLUEGIGA_I2C_H_
