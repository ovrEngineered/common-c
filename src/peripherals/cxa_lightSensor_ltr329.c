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
#define I2C_ADDR_7BIT				0x29
#define STARTUP_TIME_MS				500
#define WAKEUP_TIME_MS				500

#define TARGET_GAIN					GAIN_8X


// ******** local type definitions ********
typedef enum
{
	STATE_STANDBY,
	STATE_START_ACTIVE,
	STATE_WAIT_WAKEUP,
	STATE_ACTIVE_IDLE,
	STATE_ACTIVE_READ
}state_t;


typedef enum
{
	GAIN_1X = 0,
	GAIN_2X = 1,
	GAIN_4X = 2,
	GAIN_8X = 3,
	GAIN_48X = 6,
	GAIN_96X = 7
}gain_t;


// ******** local function prototypes ********
static bool scm_requestNewValue(cxa_lightSensor_t *const superIn);

static void stateCb_startActive_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_activeRead_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);

static void i2cCb_onWriteComplete_startup(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, void* userVarIn);

static void i2cCb_onReadComplete_ir0(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const readBytesIn, void* userVarIn);
static void i2cCb_onReadComplete_ir1(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const readBytesIn, void* userVarIn);
static void i2cCb_onReadComplete_vis0(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const readBytesIn, void* userVarIn);
static void i2cCb_onReadComplete_vis1(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const readBytesIn, void* userVarIn);



// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_lightSensor_ltr329_init(cxa_lightSensor_ltr329_t *const lightSnsIn, cxa_i2cMaster_t *const i2cIn, int threadIdIn)
{
	cxa_assert(lightSnsIn);
	cxa_assert(i2cIn);

	// save our references
	lightSnsIn->i2c = i2cIn;

	cxa_stateMachine_init(&lightSnsIn->stateMachine, "ltr329", threadIdIn);
	cxa_stateMachine_addState_timed(&lightSnsIn->stateMachine, STATE_STANDBY, "standby", STATE_START_ACTIVE, STARTUP_TIME_MS, NULL, NULL, NULL, (void*)lightSnsIn);
	cxa_stateMachine_addState(&lightSnsIn->stateMachine, STATE_START_ACTIVE, "startActive", stateCb_startActive_enter, NULL, NULL, (void*)lightSnsIn);
	cxa_stateMachine_addState_timed(&lightSnsIn->stateMachine, STATE_WAIT_WAKEUP, "wakeup", STATE_ACTIVE_IDLE, WAKEUP_TIME_MS, NULL, NULL, NULL, (void*)lightSnsIn);
	cxa_stateMachine_addState(&lightSnsIn->stateMachine, STATE_ACTIVE_IDLE, "activeIde", NULL, NULL, NULL, (void*)lightSnsIn);
	cxa_stateMachine_addState(&lightSnsIn->stateMachine, STATE_ACTIVE_READ, "activeRead", stateCb_activeRead_enter, NULL, NULL, (void*)lightSnsIn);
	cxa_stateMachine_setInitialState(&lightSnsIn->stateMachine, STATE_STANDBY);

	// initialize our superclass
	cxa_lightSensor_init(&lightSnsIn->super, scm_requestNewValue);
}


// ******** local function implementations ********
static bool scm_requestNewValue(cxa_lightSensor_t *const superIn)
{
	cxa_lightSensor_ltr329_t* lightSnsIn = (cxa_lightSensor_ltr329_t*)superIn;
	cxa_assert(lightSnsIn);

	if( cxa_stateMachine_getCurrentState(&lightSnsIn->stateMachine) != STATE_ACTIVE_IDLE ) return false;

	cxa_stateMachine_transition(&lightSnsIn->stateMachine, STATE_ACTIVE_READ);
	return true;
}


static void stateCb_startActive_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_lightSensor_ltr329_t* lightSnsIn = (cxa_lightSensor_ltr329_t*)userVarIn;
	cxa_assert(lightSnsIn);

	cxa_fixedByteBuffer_t fbb_payload;
	uint8_t fbb_payload_raw[2];
	cxa_fixedByteBuffer_initStd(&fbb_payload, fbb_payload_raw);
	cxa_fixedByteBuffer_append_uint8(&fbb_payload, 0x80);
	cxa_fixedByteBuffer_append_uint8(&fbb_payload, (TARGET_GAIN << 2) | 1);

	cxa_i2cMaster_writeBytes(lightSnsIn->i2c, I2C_ADDR_7BIT, true, &fbb_payload, i2cCb_onWriteComplete_startup, (void*)lightSnsIn);
}


static void stateCb_activeRead_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_lightSensor_ltr329_t* lightSnsIn = (cxa_lightSensor_ltr329_t*)userVarIn;
	cxa_assert(lightSnsIn);

	cxa_fixedByteBuffer_t fbb_control;
	uint8_t fbb_control_raw[1];
	cxa_fixedByteBuffer_initStd(&fbb_control, fbb_control_raw);
	cxa_fixedByteBuffer_append_uint8(&fbb_control, 0x88);
	cxa_i2cMaster_readBytes_withControlBytes(lightSnsIn->i2c, I2C_ADDR_7BIT, true, &fbb_control, 1, i2cCb_onReadComplete_ir0, (void*)lightSnsIn);
}


static void i2cCb_onWriteComplete_startup(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, void* userVarIn)
{
	cxa_lightSensor_ltr329_t* lightSnsIn = (cxa_lightSensor_ltr329_t*)userVarIn;
	cxa_assert(lightSnsIn);


	if( !wasSuccessfulIn )
	{
		cxa_stateMachine_transitionNow(&lightSnsIn->stateMachine, STATE_STANDBY);
		cxa_lightSensor_notify_updatedValue(&lightSnsIn->super, false, 0);
		return;
	}

	// if we made it here we were successful
	cxa_stateMachine_transition(&lightSnsIn->stateMachine, STATE_WAIT_WAKEUP);
}



static void i2cCb_onReadComplete_ir0(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const readBytesIn, void* userVarIn)
{
	cxa_lightSensor_ltr329_t* lightSnsIn = (cxa_lightSensor_ltr329_t*)userVarIn;
	cxa_assert(lightSnsIn);

	if( !wasSuccessfulIn )
	{
		cxa_stateMachine_transitionNow(&lightSnsIn->stateMachine, STATE_STANDBY);
		cxa_lightSensor_notify_updatedValue(&lightSnsIn->super, false, 0);
		return;
	}

	// don't really care about ir...continue...
	cxa_fixedByteBuffer_t fbb_control;
	uint8_t fbb_control_raw[1];
	cxa_fixedByteBuffer_initStd(&fbb_control, fbb_control_raw);
	cxa_fixedByteBuffer_append_uint8(&fbb_control, 0x89);
	cxa_i2cMaster_readBytes_withControlBytes(lightSnsIn->i2c, I2C_ADDR_7BIT, true, &fbb_control, 1, i2cCb_onReadComplete_ir1, (void*)lightSnsIn);
}


static void i2cCb_onReadComplete_ir1(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const readBytesIn, void* userVarIn)
{
	cxa_lightSensor_ltr329_t* lightSnsIn = (cxa_lightSensor_ltr329_t*)userVarIn;
	cxa_assert(lightSnsIn);

	if( !wasSuccessfulIn )
	{
		cxa_stateMachine_transitionNow(&lightSnsIn->stateMachine, STATE_STANDBY);
		cxa_lightSensor_notify_updatedValue(&lightSnsIn->super, false, 0);
		return;
	}

	// don't really care about ir...continue...
	cxa_fixedByteBuffer_t fbb_control;
	uint8_t fbb_control_raw[1];
	cxa_fixedByteBuffer_initStd(&fbb_control, fbb_control_raw);
	cxa_fixedByteBuffer_append_uint8(&fbb_control, 0x8A);
	cxa_i2cMaster_readBytes_withControlBytes(lightSnsIn->i2c, I2C_ADDR_7BIT, true, &fbb_control, 1, i2cCb_onReadComplete_vis0, (void*)lightSnsIn);
}


static void i2cCb_onReadComplete_vis0(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const readBytesIn, void* userVarIn)
{
	cxa_lightSensor_ltr329_t* lightSnsIn = (cxa_lightSensor_ltr329_t*)userVarIn;
	cxa_assert(lightSnsIn);

	if( !wasSuccessfulIn )
	{
		cxa_stateMachine_transitionNow(&lightSnsIn->stateMachine, STATE_STANDBY);
		cxa_lightSensor_notify_updatedValue(&lightSnsIn->super, false, 0);
		return;
	}

	// this is the MSB of the visible spectrum...record it
	uint8_t readVal;
	cxa_fixedByteBuffer_get_uint8(readBytesIn, 0, readVal);
	lightSnsIn->readVal = (((uint16_t)readVal) << 8);

	// get the last byte of visible data
	cxa_fixedByteBuffer_t fbb_control;
	uint8_t fbb_control_raw[1];
	cxa_fixedByteBuffer_initStd(&fbb_control, fbb_control_raw);
	cxa_fixedByteBuffer_append_uint8(&fbb_control, 0x8B);
	cxa_i2cMaster_readBytes_withControlBytes(lightSnsIn->i2c, I2C_ADDR_7BIT, true, &fbb_control, 1, i2cCb_onReadComplete_vis1, (void*)lightSnsIn);
}


static void i2cCb_onReadComplete_vis1(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const readBytesIn, void* userVarIn)
{
	cxa_lightSensor_ltr329_t* lightSnsIn = (cxa_lightSensor_ltr329_t*)userVarIn;
	cxa_assert(lightSnsIn);

	if( !wasSuccessfulIn )
	{
		cxa_stateMachine_transitionNow(&lightSnsIn->stateMachine, STATE_STANDBY);
		cxa_lightSensor_notify_updatedValue(&lightSnsIn->super, false, 0);
		return;
	}

	// this is the LSB of the visible spectrum...record it
	uint8_t readVal;
	cxa_fixedByteBuffer_get_uint8(readBytesIn, 0, readVal);
	lightSnsIn->readVal |= (((uint16_t)readVal) << 0);

	// transition our state in case anyone wants to read again
	cxa_stateMachine_transitionNow(&lightSnsIn->stateMachine, STATE_ACTIVE_IDLE);

	// notify with the MSB
	cxa_lightSensor_notify_updatedValue(&lightSnsIn->super, true, (lightSnsIn->readVal >> 8));
}
