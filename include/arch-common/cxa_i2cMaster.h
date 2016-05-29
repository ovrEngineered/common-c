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
#ifndef CXA_I2C_MASTER_H_
#define CXA_I2C_MASTER_H_


// ******** includes ********
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <cxa_timeDiff.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct
{
	void* _placeHolder;

	cxa_timeDiff_t td_timeout;
}cxa_i2cMaster_t;


// ******** global function prototypes ********
bool cxa_i2cMaster_writeBytes(cxa_i2cMaster_t *const i2cIn, uint8_t addressIn,
							  uint8_t* ctrlBytesIn, size_t numCtrlBytesIn,
							  uint8_t* dataBytesIn, size_t numDataBytesIn);
bool cxa_i2cMaster_readBytes(cxa_i2cMaster_t *const i2cIn, uint8_t addressIn,
							 uint8_t* ctrlBytesIn, size_t numCtrlBytesIn,
							 uint8_t* dataBytesOut, size_t numDataBytesIn);

#endif // CXA_I2C_MASTER_H_
