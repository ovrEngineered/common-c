/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_numberUtils.h"


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
