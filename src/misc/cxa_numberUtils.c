/**
 * Copyright 2015 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "cxa_numberUtils.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
uint16_t cxa_numberUtils_crc16_oneShot(void* dataIn, size_t dataLen_bytesIn)
{
	uint16_t retVal = 0;

	for( size_t i = 0; i < dataLen_bytesIn; i++ )
	{
		retVal = cxa_numberUtils_crc16_step(retVal, ((uint8_t*)dataIn)[i]);
	}

	return retVal;
}


uint16_t cxa_numberUtils_crc16_step(uint16_t crcIn, uint8_t byteIn)
{
	crcIn ^= byteIn;
	for( uint8_t i = 0; i < 8; i++ )
	{
		crcIn = (crcIn & 0x0001) ? ((crcIn >> 1) ^ 0xA001) : (crcIn >> 1);
	}
	return crcIn;
}


// ******** local function implementations ********

