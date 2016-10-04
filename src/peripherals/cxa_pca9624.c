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


#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********
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
}register_t;


typedef enum
{
	LEDOUT_OFF = 0x00,
	LEDOUT_ON = 0x01,
	LEDOUT_PWM_INDIV = 0x02,
	LEDOUT_PWM_ALL = 0x03
}ledOut_state_t;


// ******** local function prototypes ********
static bool readFromRegister(cxa_pca9624_t *const pcaIn, register_t registerIn, uint8_t* valOut);
static bool syncRegisterIfChanged(cxa_pca9624_t *const pcaIn, register_t registerIn, uint8_t valIn);
static bool writeAllRegs(cxa_pca9624_t *const pcaIn);
static bool setAllChannelsToState(cxa_pca9624_t *const pcaIn, ledOut_state_t stateIn, cxa_pca9624_channelEntry_t* chansEntriesIn, size_t numChansIn);
static bool writeAllLedBrightnesses(cxa_pca9624_t *const pcaIn, uint8_t* brightnessesIn);


// ********  local variable declarations *********


// ******** global function implementations ********
bool cxa_pca9624_init(cxa_pca9624_t *const pcaIn, cxa_i2cMaster_t *const i2cIn, uint8_t addressIn, cxa_gpio_t *const gpio_oeIn)
{
	cxa_assert(pcaIn);
	cxa_assert(i2cIn);
	cxa_assert(gpio_oeIn);

	// save our references
	pcaIn->address = addressIn;
	pcaIn->i2c = i2cIn;
	pcaIn->gpio_outputEnable = gpio_oeIn;
	pcaIn->isInit = false;

	// read MODE2 just to make sure the device is actually there
	uint8_t mode2 = 0x00;
	if( !readFromRegister(pcaIn, REG_MODE2, &mode2) ||
			(((mode2 >> 7) & 0x01) != 0) ||
			(((mode2 >> 6) & 0x01) != 0) ||
			(((mode2 >> 4) & 0x01) != 0) ||
			(((mode2 >> 2) & 0x01) != 1) ||
			(((mode2 >> 1) & 0x01) != 0) ||
			(((mode2 >> 0) & 0x01) != 1) ) return false;

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
	if( !writeAllRegs(pcaIn) ) return false;

	// enable our outputs
	cxa_gpio_setValue(pcaIn->gpio_outputEnable, 1);
	pcaIn->isInit = true;

	return true;
}


bool cxa_pca9624_setGlobalBlinkRate(cxa_pca9624_t *const pcaIn, uint16_t onPeriod_msIn, uint16_t offPeriod_msIn)
{
	cxa_assert(pcaIn);

	// make sure we're blinking (not dimming)
	uint8_t newMode2 = pcaIn->currRegs[REG_MODE2] | (1 << 5);
	if( !syncRegisterIfChanged(pcaIn, REG_MODE2, newMode2) ) return false;

	// set our duty cycle
	float dutyCycle_pcnt = ((float)onPeriod_msIn) / (((float)onPeriod_msIn) + ((float)offPeriod_msIn));
	uint16_t newGrpPwm = dutyCycle_pcnt * 255.0;
	if( !syncRegisterIfChanged(pcaIn, REG_GRPPWM, newGrpPwm) ) return false;

	// and our frequency
	uint16_t newGrpFreq = (((((float)onPeriod_msIn) / 1000.0) + (((float)offPeriod_msIn) / 1000.0)) * 24.0) - 1;
	if( !syncRegisterIfChanged(pcaIn, REG_GRPFREQ, newGrpFreq) ) return false;

	return true;
}


bool cxa_pca9624_setBrightnessForChannels(cxa_pca9624_t *const pcaIn, cxa_pca9624_channelEntry_t* chansEntriesIn, size_t numChansIn)
{
	cxa_assert(pcaIn);
	cxa_assert(chansEntriesIn);
	cxa_assert(numChansIn <= CXA_PCA9624_NUM_CHANNELS);
	if( numChansIn ==  0 ) return true;

	// make sure the channels are no longer blinking
	if( !setAllChannelsToState(pcaIn, LEDOUT_PWM_INDIV, chansEntriesIn, numChansIn) ) return false;

	// create a local copy of our registers (in case we fail)
	uint8_t newBrightness[CXA_PCA9624_NUM_CHANNELS];
	memcpy(newBrightness, &pcaIn->currRegs[REG_PWM0], sizeof(newBrightness));

	// set our new brightnesses
	for( size_t i = 0; i < numChansIn; i++ )
	{
		cxa_pca9624_channelEntry_t* currEntry = &chansEntriesIn[i];
		cxa_assert(currEntry->channelIndex < CXA_PCA9624_NUM_CHANNELS);

		newBrightness[currEntry->channelIndex] = currEntry->brightness;
	}

	// now write to the controller
	if( !writeAllLedBrightnesses(pcaIn, newBrightness) ) return false;

	// if we made it here, the write was successful...store our new values
	memcpy(&pcaIn->currRegs[REG_PWM0], newBrightness, sizeof(newBrightness));
	return true;
}


bool cxa_pca9624_blinkChannels(cxa_pca9624_t *const pcaIn, cxa_pca9624_channelEntry_t* chansEntriesIn, size_t numChansIn)
{
	cxa_assert(pcaIn);
	cxa_assert(chansEntriesIn);
	cxa_assert(numChansIn <= CXA_PCA9624_NUM_CHANNELS);
	if( numChansIn == 0 ) return true;

	// have to set our brightnesses first
	if( !cxa_pca9624_setBrightnessForChannels(pcaIn, chansEntriesIn, numChansIn) ) return false;

	// now we need to tell the channels to blink
	return setAllChannelsToState(pcaIn, LEDOUT_PWM_ALL, chansEntriesIn, numChansIn);
}


// ******** local function implementations ********
static bool readFromRegister(cxa_pca9624_t *const pcaIn, register_t registerIn, uint8_t* valOut)
{
	cxa_assert(pcaIn);

	uint8_t ctrlBytes = registerIn;
	return cxa_i2cMaster_readBytes(pcaIn->i2c, pcaIn->address, &ctrlBytes, 1, valOut, 1);
}


static bool syncRegisterIfChanged(cxa_pca9624_t *const pcaIn, register_t registerIn, uint8_t valIn)
{
	cxa_assert(pcaIn);

	// see if we're already at the target value
	if( pcaIn->currRegs[registerIn] == valIn ) return true;

	// if we made it here, we need to do a write
	uint8_t ctrlBytes = registerIn;
	bool retVal = cxa_i2cMaster_writeBytes(pcaIn->i2c, pcaIn->address, &ctrlBytes, 1, &valIn, 1);

	// if the write was successful, update our local copy
	if( retVal ) pcaIn->currRegs[registerIn] = valIn;

	return retVal;
}


static bool writeAllRegs(cxa_pca9624_t *const pcaIn)
{
	cxa_assert(pcaIn);

	uint8_t ctrlBytes = 0x80;
	return cxa_i2cMaster_writeBytes(pcaIn->i2c, pcaIn->address, &ctrlBytes, 1, pcaIn->currRegs, 18);
}


static bool setAllChannelsToState(cxa_pca9624_t *const pcaIn, ledOut_state_t stateIn, cxa_pca9624_channelEntry_t* chansEntriesIn, size_t numChansIn)
{
	cxa_assert(pcaIn);
	cxa_assert(chansEntriesIn);
	cxa_assert(numChansIn <= CXA_PCA9624_NUM_CHANNELS);
	if( numChansIn == 0 ) return true;

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

	return syncRegisterIfChanged(pcaIn, REG_LEDOUT0, newLedOut0) && syncRegisterIfChanged(pcaIn, REG_LEDOUT1, newLedOut1);
}


static bool writeAllLedBrightnesses(cxa_pca9624_t *const pcaIn, uint8_t* brightnessesIn)
{
	cxa_assert(pcaIn);

	uint8_t ctrlBytes = 0xA0 | REG_PWM0;
	return cxa_i2cMaster_writeBytes(pcaIn->i2c, pcaIn->address, &ctrlBytes, 1, brightnessesIn, CXA_PCA9624_NUM_CHANNELS);
}
