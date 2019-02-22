/**
 * Copyright 2016 opencxa.org
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
#include "cxa_pca9624.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdio.h>
#include <string.h>
#include <cxa_assert.h>
#include <cxa_stateMachine.h>


#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********
typedef enum
{
	STATE_UNINIT,
	STATE_VERIFY_COMMS,
	STATE_INIT_CONFIG,
	STATE_RUNNING,
	STATE_ERROR
}state_t;


typedef enum
{
	REG_MODE1 = 0x00,
	REG_MODE2 = 0x01,
	REG_PWM0 = 0x02,
	REG_PWM1 = 0x03,
	REG_PWM2 = 0x04,
	REG_PWM3 = 0x05,
	REG_PWM4 = 0x06,
	REG_PWM5 = 0x07,
	REG_PWM6 = 0x08,
	REG_PWM7 = 0x09,
	REG_GRPPWM = 0x0A,
	REG_GRPFREQ = 0x0B,
	REG_LEDOUT0 = 0x0C,
	REG_LEDOUT1 = 0x0D,
	REG_SUBADR1 = 0x0E,
	REG_SUBADR2 = 0x0F,
	REG_SUBADR3 = 0x10,
	REG_ALLCALLADR = 0x11
}pca_register_t;


typedef enum
{
	LEDOUT_OFF = 0x00,
	LEDOUT_ON = 0x01,
	LEDOUT_PWM_INDIV = 0x02,
	LEDOUT_PWM_ALL = 0x03
}ledOut_state_t;


// ******** local function prototypes ********
static void writeAllRegs(cxa_pca9624_t *const pcaIn);
static void setChannelsToState(cxa_pca9624_t *const pcaIn, ledOut_state_t stateIn, cxa_pca9624_channelEntry_t* chansEntriesIn, size_t numChansIn);

static void stateCb_verifyComms_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_initConfigure_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_running_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_error_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);

static void i2cCb_onReadComplete_verifyComms(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const readBytesIn, void* userVarIn);
static void i2cCb_onWriteComplete_allRegs(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_pca9624_init(cxa_pca9624_t *const pcaIn, cxa_i2cMaster_t *const i2cIn, uint8_t addressIn, cxa_gpio_t *const gpio_oeIn, int threadIdIn)
{
	cxa_assert(pcaIn);
	cxa_assert(i2cIn);
	cxa_assert(gpio_oeIn);

	// save our references
	pcaIn->address = addressIn;
	pcaIn->i2c = i2cIn;
	pcaIn->gpio_outputEnable = gpio_oeIn;

	// initialize our listeners
	cxa_array_initStd(&pcaIn->listeners, pcaIn->listeners_raw);

	// initialize our logger
	cxa_logger_init(&pcaIn->logger, "pca9624");

	// initialize our stateMachine
	cxa_stateMachine_init(&pcaIn->stateMachine, "pca9624", threadIdIn);
	cxa_stateMachine_addState(&pcaIn->stateMachine, STATE_UNINIT, "uninit", NULL, NULL, NULL, (void*)pcaIn);
	cxa_stateMachine_addState(&pcaIn->stateMachine, STATE_VERIFY_COMMS, "verifyComms", stateCb_verifyComms_enter, NULL, NULL, (void*)pcaIn);
	cxa_stateMachine_addState(&pcaIn->stateMachine, STATE_INIT_CONFIG, "initConfig", stateCb_initConfigure_enter, NULL, NULL, (void*)pcaIn);
	cxa_stateMachine_addState(&pcaIn->stateMachine, STATE_RUNNING, "running", stateCb_running_enter, NULL, NULL, (void*)pcaIn);
	cxa_stateMachine_addState(&pcaIn->stateMachine, STATE_ERROR, "error", stateCb_error_enter, NULL, NULL, (void*)pcaIn);
	cxa_stateMachine_setInitialState(&pcaIn->stateMachine, STATE_UNINIT);
}


void cxa_pca9624_addListener(cxa_pca9624_t *const pcaIn, cxa_pca9624_cb_t cb_onBecomesReadyIn, cxa_pca9624_cb_t cb_onErrorIn, void *userVarIn)
{
	cxa_assert(pcaIn);

	cxa_pca9624_listener_t newListener = {
			.cb_onBecomesReady = cb_onBecomesReadyIn,
			.cb_onError = cb_onErrorIn,
			.userVar = userVarIn
	};

	cxa_assert(cxa_array_append(&pcaIn->listeners, &newListener));
}


void cxa_pca9624_start(cxa_pca9624_t *const pcaIn)
{
	cxa_assert(pcaIn);
	if( cxa_stateMachine_getCurrentState(&pcaIn->stateMachine) != STATE_UNINIT ) return;

	cxa_stateMachine_transition(&pcaIn->stateMachine, STATE_VERIFY_COMMS);
}


bool cxa_pca9624_hasStartedSuccessfully(cxa_pca9624_t *const pcaIn)
{
	return (cxa_stateMachine_getCurrentState(&pcaIn->stateMachine) == STATE_RUNNING);
}


void cxa_pca9624_setBrightnessForChannels(cxa_pca9624_t *const pcaIn, cxa_pca9624_channelEntry_t* chansEntriesIn, size_t numChansIn)
{
	cxa_assert(pcaIn);
	cxa_assert(chansEntriesIn);
	cxa_assert(numChansIn <= CXA_PCA9624_NUM_CHANNELS);
	if( cxa_stateMachine_getCurrentState(&pcaIn->stateMachine) != STATE_RUNNING ) return;
	if( numChansIn ==  0 ) return;

	// make sure the channels are no longer blinking
	setChannelsToState(pcaIn, LEDOUT_PWM_INDIV, chansEntriesIn, numChansIn);

	// set our new brightnesses
	for( size_t i = 0; i < numChansIn; i++ )
	{
		cxa_pca9624_channelEntry_t* currEntry = &chansEntriesIn[i];
		cxa_assert(currEntry->channelIndex < CXA_PCA9624_NUM_CHANNELS);

		pcaIn->currRegs[REG_PWM0 + currEntry->channelIndex] = currEntry->brightness;
	}

	// now write to the controller
	writeAllRegs(pcaIn);
}


void cxa_pca9624_blinkChannels(cxa_pca9624_t *const pcaIn, cxa_pca9624_channelEntry_t* chansEntriesIn, size_t numChansIn)
{
	cxa_assert(pcaIn);
	cxa_assert(chansEntriesIn);
	cxa_assert(numChansIn <= CXA_PCA9624_NUM_CHANNELS);
	if( cxa_stateMachine_getCurrentState(&pcaIn->stateMachine) != STATE_RUNNING ) return;
	if( numChansIn == 0 ) return;

	// have to set our brightnesses first
	cxa_pca9624_setBrightnessForChannels(pcaIn, chansEntriesIn, numChansIn);

	// now we need to tell the channels to blink
	setChannelsToState(pcaIn, LEDOUT_PWM_ALL, chansEntriesIn, numChansIn);
}


// ******** local function implementations ********
static void writeAllRegs(cxa_pca9624_t *const pcaIn)
{
	cxa_fixedByteBuffer_t fbb_writeBytes;
	uint8_t fbb_writeBytes_raw[sizeof(pcaIn->currRegs)+1];
	cxa_fixedByteBuffer_initStd(&fbb_writeBytes, fbb_writeBytes_raw);
	cxa_fixedByteBuffer_append_uint8(&fbb_writeBytes, 0x80);
	cxa_fixedByteBuffer_append(&fbb_writeBytes, pcaIn->currRegs, sizeof(pcaIn->currRegs));

	cxa_i2cMaster_writeBytes(pcaIn->i2c, pcaIn->address, true, &fbb_writeBytes, i2cCb_onWriteComplete_allRegs, (void*)pcaIn);
}


static void setChannelsToState(cxa_pca9624_t *const pcaIn, ledOut_state_t stateIn, cxa_pca9624_channelEntry_t* chansEntriesIn, size_t numChansIn)
{
	cxa_assert(pcaIn);
	cxa_assert(chansEntriesIn);
	cxa_assert(numChansIn <= CXA_PCA9624_NUM_CHANNELS);
	if( numChansIn == 0 ) return;

	// now we need to tell the channels to blink
	uint8_t newLedOut0 = pcaIn->currRegs[REG_LEDOUT0];
	uint8_t newLedOut1 = pcaIn->currRegs[REG_LEDOUT1];
	for( size_t i = 0; i < numChansIn; i++ )
	{
		cxa_pca9624_channelEntry_t* currEntry = &chansEntriesIn[i];

		uint8_t* targetLedOut = (currEntry->channelIndex < 4) ? &newLedOut0 : &newLedOut1;
		uint8_t shiftAmount = (currEntry->channelIndex % 4) * 2;

		*targetLedOut = (*targetLedOut & ~(0x03 << shiftAmount)) | (stateIn << shiftAmount);
	}

	pcaIn->currRegs[REG_LEDOUT0] = newLedOut0;
	pcaIn->currRegs[REG_LEDOUT1] = newLedOut1;
}


static void stateCb_verifyComms_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_pca9624_t *const pcaIn = (cxa_pca9624_t *const)userVarIn;
	cxa_assert(pcaIn);

	// read MODE2 just to make sure the device is actually there

	// write the control register...follow-up read will be completed in callback
	cxa_fixedByteBuffer_t fbb_writeBytes;
	uint8_t fbb_writeBytes_raw[1];
	cxa_fixedByteBuffer_initStd(&fbb_writeBytes, fbb_writeBytes_raw);
	cxa_fixedByteBuffer_append_uint8(&fbb_writeBytes, (uint8_t)REG_MODE2);

	cxa_i2cMaster_readBytes_withControlBytes(pcaIn->i2c, pcaIn->address, true, &fbb_writeBytes, 1, i2cCb_onReadComplete_verifyComms, (void*)pcaIn);
}


static void stateCb_initConfigure_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_pca9624_t *const pcaIn = (cxa_pca9624_t *const)userVarIn;
	cxa_assert(pcaIn);

	// if we made it here, it looks like we have a responding, valid device...configure it
	pcaIn->currRegs[REG_MODE1] = 0x01;
	pcaIn->currRegs[REG_MODE2] = 0x05;
	pcaIn->currRegs[REG_PWM0] = 0x00;
	pcaIn->currRegs[REG_PWM1] = 0x00;
	pcaIn->currRegs[REG_PWM2] = 0x00;
	pcaIn->currRegs[REG_PWM3] = 0x00;
	pcaIn->currRegs[REG_PWM4] = 0x00;
	pcaIn->currRegs[REG_PWM5] = 0x00;
	pcaIn->currRegs[REG_PWM6] = 0x00;
	pcaIn->currRegs[REG_PWM7] = 0x00;
	pcaIn->currRegs[REG_GRPPWM] = 0xFF;
	pcaIn->currRegs[REG_GRPFREQ] = 0x00;
	pcaIn->currRegs[REG_LEDOUT0] = 0xAA;
	pcaIn->currRegs[REG_LEDOUT1] = 0xAA;
	pcaIn->currRegs[REG_SUBADR1] = 0xE2;
	pcaIn->currRegs[REG_SUBADR2] = 0xE4;
	pcaIn->currRegs[REG_SUBADR3] = 0xE8;
	pcaIn->currRegs[REG_ALLCALLADR] = 0xE0;

	// write the control register first...then everything else
	writeAllRegs(pcaIn);
}


static void stateCb_running_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_pca9624_t *const pcaIn = (cxa_pca9624_t *const)userVarIn;
	cxa_assert(pcaIn);

	cxa_logger_info(&pcaIn->logger, "pca9624 ready");

	cxa_array_iterate(&pcaIn->listeners, currListener, cxa_pca9624_listener_t)
	{
		if( (currListener != NULL) && (currListener->cb_onBecomesReady != NULL) ) currListener->cb_onBecomesReady(currListener->userVar);
	}
}


static void stateCb_error_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_pca9624_t *const pcaIn = (cxa_pca9624_t *const)userVarIn;
	cxa_assert(pcaIn);

	cxa_array_iterate(&pcaIn->listeners, currListener, cxa_pca9624_listener_t)
	{
		if( (currListener != NULL) && (currListener->cb_onError != NULL) ) currListener->cb_onError(currListener->userVar);
	}
}


static void i2cCb_onReadComplete_verifyComms(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const readBytesIn, void* userVarIn)
{
	cxa_pca9624_t *const pcaIn = (cxa_pca9624_t *const)userVarIn;
	cxa_assert(pcaIn);
	cxa_assert( cxa_stateMachine_getCurrentState(&pcaIn->stateMachine) == STATE_VERIFY_COMMS );

	uint8_t mode2;
	if( !wasSuccessfulIn || (cxa_fixedByteBuffer_getSize_bytes(readBytesIn) != 1) ||
		!cxa_fixedByteBuffer_get_uint8(readBytesIn, 0, mode2) ||
		(((mode2 >> 7) & 0x01) != 0) ||
		(((mode2 >> 6) & 0x01) != 0) ||
		(((mode2 >> 4) & 0x01) != 0) ||
		(((mode2 >> 2) & 0x01) != 1) ||
		(((mode2 >> 1) & 0x01) != 0) ||
		(((mode2 >> 0) & 0x01) != 1) )
	{
		cxa_logger_error(&pcaIn->logger, "error verifying pca9624");
		cxa_stateMachine_transition(&pcaIn->stateMachine, STATE_ERROR);
		return;
	}

	// if we made it here, we read the byte successfully...start initialization
	cxa_stateMachine_transition(&pcaIn->stateMachine, STATE_INIT_CONFIG);
}


static void i2cCb_onWriteComplete_allRegs(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, void* userVarIn)
{
	cxa_pca9624_t *const pcaIn = (cxa_pca9624_t *const)userVarIn;
	cxa_assert(pcaIn);

	if( !wasSuccessfulIn )
	{
		cxa_logger_error(&pcaIn->logger, "error writing pca9624 registers");
		cxa_stateMachine_transition(&pcaIn->stateMachine, STATE_ERROR);
		return;
	}

	// if we made it here, write was successful
	if( cxa_stateMachine_getCurrentState(&pcaIn->stateMachine) == STATE_INIT_CONFIG )
	{
		// if we made it here, we configured successfully...enable our outputs
		cxa_gpio_setValue(pcaIn->gpio_outputEnable, 1);
		cxa_stateMachine_transition(&pcaIn->stateMachine, STATE_RUNNING);
	}
}
