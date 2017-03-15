/**
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
#include "cxa_tempSensor_si7050.h"


// ******** includes ********
#include <math.h>

#include <cxa_assert.h>
#include <cxa_fixedByteBuffer.h>

#define CXA_LOG_LEVEL				CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define I2C_ADDR					0x80


// ******** local type definitions ********


// ******** local function prototypes ********
static bool scm_requestNewValue(cxa_tempSensor_t *const superIn);

static void i2cCb_onReadComplete(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const readBytesIn, void* userVarIn);
static void i2cCb_onWriteComplete(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, void* userVarIn);



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
static bool scm_requestNewValue(cxa_tempSensor_t *const superIn)
{
	cxa_tempSensor_si7050_t* siIn = (cxa_tempSensor_si7050_t*)superIn;
	cxa_assert(siIn);

	// BGScript command:
	// call hardware_i2c_write($80, 0, 1, "\xE3")

	cxa_fixedByteBuffer_t fbb_payload;
	uint8_t fbb_payload_raw[1];
	cxa_fixedByteBuffer_initStd(&fbb_payload, fbb_payload_raw);
	if( !cxa_fixedByteBuffer_append_uint8(&fbb_payload, 0xE3) ) return false;

	cxa_i2cMaster_writeBytes(siIn->i2c, I2C_ADDR, 0, &fbb_payload, i2cCb_onWriteComplete, (void*)siIn);

	return true;
}


static void i2cCb_onReadComplete(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const readBytesIn, void* userVarIn)
{
	cxa_tempSensor_si7050_t* siIn = (cxa_tempSensor_si7050_t*)userVarIn;
	cxa_assert(siIn);

	if( !siIn->wasWriteSuccessful ) wasSuccessfulIn = false;
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


static void i2cCb_onWriteComplete(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, void* userVarIn)
{
	cxa_tempSensor_si7050_t* siIn = (cxa_tempSensor_si7050_t*)userVarIn;
	cxa_assert(siIn);

	// BGScript command:
	// call hardware_i2c_read($80, 1, 2)(result, data_len, data(:))
	siIn->wasWriteSuccessful = wasSuccessfulIn;

	// we need to call this anyways...otherwise I2C bus will be left busy
	cxa_i2cMaster_readBytes(siIn->i2c, I2C_ADDR, true, 2, i2cCb_onReadComplete, (void*)siIn);
}
