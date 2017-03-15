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
#include "cxa_lightSensor_ltr329.h"


// ******** includes ********
#include <math.h>

#include <cxa_assert.h>
#include <cxa_fixedByteBuffer.h>

#define CXA_LOG_LEVEL				CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define I2C_ADDR					0x52
#define STARTUP_TIME_MS				2000


// ******** local type definitions ********
typedef enum
{
	STATE_STANDBY,
	STATE_STARTMEASURE,
	STATE_WRITE_IR_BYTE0,
	STATE_READ_IR_BYTE0,
	STATE_WRITE_IR_BYTE1,
	STATE_READ_IR_BYTE1,
	STATE_WRITE_VIS_BYTE0,
	STATE_READ_VIS_BYTE0,
	STATE_WRITE_VIS_BYTE1,
	STATE_READ_VIS_BYTE1,
	STATE_GOTO_STANDBY
}state_t;


// ******** local function prototypes ********
static bool scm_requestNewValue(cxa_lightSensor_t *const superIn);

static void stateCb_standby_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_startMeasure_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_writeIr_byte0_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_writeIr_byte1_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_writeVis_byte0_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_writeVis_byte1_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_readByte_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_gotoStandby_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);


static void i2cCb_onReadComplete(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const readBytesIn, void* userVarIn);
static void i2cCb_onWriteComplete(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, void* userVarIn);



// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_lightSensor_ltr329_init(cxa_lightSensor_ltr329_t *const lightSnsIn, cxa_i2cMaster_t *const i2cIn)
{
	cxa_assert(lightSnsIn);
	cxa_assert(i2cIn);

	// save our references
	lightSnsIn->i2c = i2cIn;

	cxa_stateMachine_init(&lightSnsIn->stateMachine, "ltr329");
	cxa_stateMachine_addState(&lightSnsIn->stateMachine, STATE_STANDBY, "standby", stateCb_standby_enter, NULL, NULL, (void*)lightSnsIn);
	cxa_stateMachine_addState_timed(&lightSnsIn->stateMachine, STATE_STARTMEASURE, "start", STATE_WRITE_IR_BYTE0, STARTUP_TIME_MS, stateCb_startMeasure_enter, NULL, NULL, (void*)lightSnsIn);
	cxa_stateMachine_addState(&lightSnsIn->stateMachine, STATE_WRITE_IR_BYTE0, "writeIr0", stateCb_writeIr_byte0_enter, NULL, NULL, (void*)lightSnsIn);
	cxa_stateMachine_addState(&lightSnsIn->stateMachine, STATE_READ_IR_BYTE0, "readIr0", stateCb_readByte_enter, NULL, NULL, (void*)lightSnsIn);
	cxa_stateMachine_addState(&lightSnsIn->stateMachine, STATE_WRITE_IR_BYTE1, "writeIr1", stateCb_writeIr_byte1_enter, NULL, NULL, (void*)lightSnsIn);
	cxa_stateMachine_addState(&lightSnsIn->stateMachine, STATE_READ_IR_BYTE1, "readIr1", stateCb_readByte_enter, NULL, NULL, (void*)lightSnsIn);
	cxa_stateMachine_addState(&lightSnsIn->stateMachine, STATE_WRITE_VIS_BYTE0, "writeVis0", stateCb_writeVis_byte0_enter, NULL, NULL, (void*)lightSnsIn);
	cxa_stateMachine_addState(&lightSnsIn->stateMachine, STATE_READ_VIS_BYTE0, "readVis0", stateCb_readByte_enter, NULL, NULL, (void*)lightSnsIn);
	cxa_stateMachine_addState(&lightSnsIn->stateMachine, STATE_WRITE_VIS_BYTE1, "writeVis1", stateCb_writeVis_byte1_enter, NULL, NULL, (void*)lightSnsIn);
	cxa_stateMachine_addState(&lightSnsIn->stateMachine, STATE_READ_VIS_BYTE1, "readVis1", stateCb_readByte_enter, NULL, NULL, (void*)lightSnsIn);
	cxa_stateMachine_addState(&lightSnsIn->stateMachine, STATE_GOTO_STANDBY, "gotoStandby", stateCb_gotoStandby_enter, NULL, NULL, (void*)lightSnsIn);
	cxa_stateMachine_setInitialState(&lightSnsIn->stateMachine, STATE_STANDBY);

	// initialize our superclass
	cxa_lightSensor_init(&lightSnsIn->super, scm_requestNewValue);
}


// ******** local function implementations ********
static bool scm_requestNewValue(cxa_lightSensor_t *const superIn)
{
	cxa_lightSensor_ltr329_t* lightSnsIn = (cxa_lightSensor_ltr329_t*)superIn;
	cxa_assert(lightSnsIn);

	if( cxa_stateMachine_getCurrentState(&lightSnsIn->stateMachine) != STATE_STANDBY ) return false;

	lightSnsIn->hasNewVal = false;
	cxa_stateMachine_transition(&lightSnsIn->stateMachine, STATE_STARTMEASURE);
	return true;
}


static void stateCb_standby_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_lightSensor_ltr329_t* lightSnsIn = (cxa_lightSensor_ltr329_t*)userVarIn;
	cxa_assert(lightSnsIn);

	if( lightSnsIn->hasNewVal )
	{
		lightSnsIn->hasNewVal = false;
		cxa_lightSensor_notify_updatedValue(&lightSnsIn->super, true, lightSnsIn->readVal >> 8);
	}
	else if( lightSnsIn->encounteredError )
	{
		lightSnsIn->encounteredError = false;
		cxa_lightSensor_notify_updatedValue(&lightSnsIn->super, false, 0);
	}
}


static void stateCb_startMeasure_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_lightSensor_ltr329_t* lightSnsIn = (cxa_lightSensor_ltr329_t*)userVarIn;
	cxa_assert(lightSnsIn);

	// BGScript command:
	// call hardware_i2c_write($52, 1, 2, "\x80\x0D")

	cxa_fixedByteBuffer_t fbb_payload;
	uint8_t fbb_payload_raw[2];
	cxa_fixedByteBuffer_initStd(&fbb_payload, fbb_payload_raw);
	cxa_fixedByteBuffer_append_uint8(&fbb_payload, 0x80);
	cxa_fixedByteBuffer_append_uint8(&fbb_payload, 0x0D);				// 8X gain

	cxa_i2cMaster_writeBytes(lightSnsIn->i2c, I2C_ADDR, true, &fbb_payload, i2cCb_onWriteComplete, (void*)lightSnsIn);
}


static void stateCb_writeIr_byte0_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_lightSensor_ltr329_t* lightSnsIn = (cxa_lightSensor_ltr329_t*)userVarIn;
	cxa_assert(lightSnsIn);

	// BGScript command:
	// call hardware_i2c_write($52, 0, 1, "\x88")

	cxa_fixedByteBuffer_t fbb_payload;
	uint8_t fbb_payload_raw[1];
	cxa_fixedByteBuffer_initStd(&fbb_payload, fbb_payload_raw);
	cxa_fixedByteBuffer_append_uint8(&fbb_payload, 0x88);

	cxa_i2cMaster_writeBytes(lightSnsIn->i2c, I2C_ADDR, false, &fbb_payload, i2cCb_onWriteComplete, (void*)lightSnsIn);
}


static void stateCb_writeIr_byte1_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_lightSensor_ltr329_t* lightSnsIn = (cxa_lightSensor_ltr329_t*)userVarIn;
	cxa_assert(lightSnsIn);

	// BGScript command:
	// call hardware_i2c_write($52, 0, 1, "\x89")

	cxa_fixedByteBuffer_t fbb_payload;
	uint8_t fbb_payload_raw[1];
	cxa_fixedByteBuffer_initStd(&fbb_payload, fbb_payload_raw);
	cxa_fixedByteBuffer_append_uint8(&fbb_payload, 0x89);

	cxa_i2cMaster_writeBytes(lightSnsIn->i2c, I2C_ADDR, false, &fbb_payload, i2cCb_onWriteComplete, (void*)lightSnsIn);
}


static void stateCb_writeVis_byte0_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_lightSensor_ltr329_t* lightSnsIn = (cxa_lightSensor_ltr329_t*)userVarIn;
	cxa_assert(lightSnsIn);

	// BGScript command:
	// call hardware_i2c_write($52, 0, 1, "\x8A")

	cxa_fixedByteBuffer_t fbb_payload;
	uint8_t fbb_payload_raw[1];
	cxa_fixedByteBuffer_initStd(&fbb_payload, fbb_payload_raw);
	cxa_fixedByteBuffer_append_uint8(&fbb_payload, 0x8A);

	cxa_i2cMaster_writeBytes(lightSnsIn->i2c, I2C_ADDR, false, &fbb_payload, i2cCb_onWriteComplete, (void*)lightSnsIn);
}


static void stateCb_writeVis_byte1_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_lightSensor_ltr329_t* lightSnsIn = (cxa_lightSensor_ltr329_t*)userVarIn;
	cxa_assert(lightSnsIn);

	// BGScript command:
	// call hardware_i2c_write($52, 0, 1, "\x8B")

	cxa_fixedByteBuffer_t fbb_payload;
	uint8_t fbb_payload_raw[1];
	cxa_fixedByteBuffer_initStd(&fbb_payload, fbb_payload_raw);
	cxa_fixedByteBuffer_append_uint8(&fbb_payload, 0x8B);

	cxa_i2cMaster_writeBytes(lightSnsIn->i2c, I2C_ADDR, false, &fbb_payload, i2cCb_onWriteComplete, (void*)lightSnsIn);
}


static void stateCb_readByte_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_lightSensor_ltr329_t* lightSnsIn = (cxa_lightSensor_ltr329_t*)userVarIn;
	cxa_assert(lightSnsIn);

	cxa_i2cMaster_readBytes(lightSnsIn->i2c, I2C_ADDR, true, 1, i2cCb_onReadComplete, (void*)lightSnsIn);
}


static void stateCb_gotoStandby_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_lightSensor_ltr329_t* lightSnsIn = (cxa_lightSensor_ltr329_t*)userVarIn;
	cxa_assert(lightSnsIn);

	// BGScript command:
	// call hardware_i2c_write($52, 1, 2, "\x80\x00")

	cxa_fixedByteBuffer_t fbb_payload;
	uint8_t fbb_payload_raw[2];
	cxa_fixedByteBuffer_initStd(&fbb_payload, fbb_payload_raw);
	cxa_fixedByteBuffer_append_uint8(&fbb_payload, 0x80);
	cxa_fixedByteBuffer_append_uint8(&fbb_payload, 0x00);

	cxa_i2cMaster_writeBytes(lightSnsIn->i2c, I2C_ADDR, 0, &fbb_payload, i2cCb_onWriteComplete, (void*)lightSnsIn);
}


static void i2cCb_onReadComplete(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const readBytesIn, void* userVarIn)
{
	cxa_lightSensor_ltr329_t* lightSnsIn = (cxa_lightSensor_ltr329_t*)userVarIn;
	cxa_assert(lightSnsIn);

	uint8_t currByte;
	if( !cxa_fixedByteBuffer_get_uint8(readBytesIn, 0, currByte) ) wasSuccessfulIn = false;

	if( !wasSuccessfulIn ) lightSnsIn->encounteredError = true;

	// depends on our state
	state_t currState = cxa_stateMachine_getCurrentState(&lightSnsIn->stateMachine);
	if( wasSuccessfulIn && (currState == STATE_READ_IR_BYTE0 ) )
	{
		cxa_stateMachine_transition(&lightSnsIn->stateMachine, STATE_WRITE_IR_BYTE1);
	}
	else if( wasSuccessfulIn && (currState == STATE_READ_IR_BYTE1 ) )
	{
		cxa_stateMachine_transition(&lightSnsIn->stateMachine, STATE_WRITE_VIS_BYTE0);
	}
	else if( wasSuccessfulIn && (currState == STATE_READ_VIS_BYTE0 ) )
	{
		lightSnsIn->readVal = (lightSnsIn->readVal & 0x00FF) | (currByte << 8);
		cxa_stateMachine_transition(&lightSnsIn->stateMachine, STATE_WRITE_VIS_BYTE1);
	}
	else if( wasSuccessfulIn && (currState == STATE_READ_VIS_BYTE1 ) )
	{
		lightSnsIn->readVal = (lightSnsIn->readVal & 0xFF00) | currByte;
		lightSnsIn->hasNewVal = true;
		cxa_stateMachine_transition(&lightSnsIn->stateMachine, STATE_GOTO_STANDBY);
	}
	else
	{
		cxa_stateMachine_transition(&lightSnsIn->stateMachine, STATE_GOTO_STANDBY);
	}
}


static void i2cCb_onWriteComplete(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, void* userVarIn)
{
	cxa_lightSensor_ltr329_t* lightSnsIn = (cxa_lightSensor_ltr329_t*)userVarIn;
	cxa_assert(lightSnsIn);

	if( !wasSuccessfulIn ) lightSnsIn->encounteredError = true;

	// depends on our state
	state_t currState = cxa_stateMachine_getCurrentState(&lightSnsIn->stateMachine);
	if( wasSuccessfulIn && (currState == STATE_STARTMEASURE ) )
	{
		// nothing to do here
	}
	else if( wasSuccessfulIn && (currState == STATE_WRITE_IR_BYTE0 ) )
	{
		cxa_stateMachine_transition(&lightSnsIn->stateMachine, STATE_READ_IR_BYTE0);
	}
	else if( wasSuccessfulIn && (currState == STATE_WRITE_IR_BYTE1 ) )
	{
		cxa_stateMachine_transition(&lightSnsIn->stateMachine, STATE_READ_IR_BYTE1);
	}
	else if( wasSuccessfulIn && (currState == STATE_WRITE_VIS_BYTE0 ) )
	{
		cxa_stateMachine_transition(&lightSnsIn->stateMachine, STATE_READ_VIS_BYTE0);
	}
	else if( wasSuccessfulIn && (currState == STATE_WRITE_VIS_BYTE1 ) )
	{
		cxa_stateMachine_transition(&lightSnsIn->stateMachine, STATE_READ_VIS_BYTE1);
	}
	else if( currState == STATE_GOTO_STANDBY )
	{
		cxa_stateMachine_transition(&lightSnsIn->stateMachine, STATE_STANDBY);
	}
	else
	{
		cxa_stateMachine_transition(&lightSnsIn->stateMachine, STATE_GOTO_STANDBY);
	}
}
