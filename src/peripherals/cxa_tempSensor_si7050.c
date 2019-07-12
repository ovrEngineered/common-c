/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_tempSensor_si7050.h"


// ******** includes ********
#include <math.h>

#include <cxa_assert.h>
#include <cxa_fixedByteBuffer.h>

#define CXA_LOG_LEVEL				CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define I2C_ADDR_7BIT					0x40


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_requestNewValue(cxa_tempSensor_t *const superIn);

static void i2cCb_onReadComplete(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const readBytesIn, void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_tempSensor_si7050_init(cxa_tempSensor_si7050_t *const siIn, cxa_i2cMaster_t *const i2cIn)
{
	cxa_assert(siIn);
	cxa_assert(i2cIn);

	// save our references
	siIn->i2c = i2cIn;

	// initialize our superclass
	cxa_tempSensor_init(&siIn->super, scm_requestNewValue);
}


// ******** local function implementations ********
static void scm_requestNewValue(cxa_tempSensor_t *const superIn)
{
	cxa_tempSensor_si7050_t* siIn = (cxa_tempSensor_si7050_t*)superIn;
	cxa_assert(siIn);

	cxa_fixedByteBuffer_t fbb_payload;
	uint8_t fbb_payload_raw[1];
	cxa_fixedByteBuffer_initStd(&fbb_payload, fbb_payload_raw);
	if( !cxa_fixedByteBuffer_append_uint8(&fbb_payload, 0xE3) )
	{
		cxa_tempSensor_notify_updatedValue(&siIn->super, false, NAN);
		return;
	}

	cxa_i2cMaster_readBytes_withControlBytes(siIn->i2c, I2C_ADDR_7BIT, true, &fbb_payload, 2, i2cCb_onReadComplete, (void*)siIn);
}


static void i2cCb_onReadComplete(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const readBytesIn, void* userVarIn)
{
	cxa_tempSensor_si7050_t* siIn = (cxa_tempSensor_si7050_t*)userVarIn;
	cxa_assert(siIn);

	if( wasSuccessfulIn && (cxa_fixedByteBuffer_getSize_bytes(readBytesIn) != 2) ) wasSuccessfulIn = false;

	float temp_c = NAN;
	if( wasSuccessfulIn )
	{
		uint16_t tempCode;
		cxa_fixedByteBuffer_get_uint16BE(readBytesIn, 0, tempCode);

		temp_c = ((175.72 * tempCode) / 65536) - 46.85;
	}

	cxa_tempSensor_notify_updatedValue(&siIn->super, wasSuccessfulIn, temp_c);
}
