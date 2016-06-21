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
}register_t;


typedef enum
{
	LEDOUT_OFF,
	LEDOUT_ON,
	LEDOUT_PWM_INDIV,
	LEDOUT_PWM_ALL
}ledOut_state_t;


// ******** local function prototypes ********
static bool readFromRegister(cxa_pca9624_t *const pcaIn, register_t registerIn, uint8_t* valOut);
static bool writeToRegister(cxa_pca9624_t *const pcaIn, register_t registerIn, uint8_t valIn);
static bool writeFullConfig(cxa_pca9624_t *const pcaIn, uint8_t* fullDevConfig);
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
	uint8_t fullDevConfig[] =
			{
				0x01,		// MODE1
				0x05,		// MODE2
				0x00,		// PWM0
				0x00,		// PWM1
				0x00,		// PWM2
				0x00,		// PWM3
				0x00,		// PWM4
				0x00,		// PWM5
				0x00,		// PWM6
				0x00,		// PWM7
				0xFF,		// GRPPWM
				0x00,		// GRPFREQ
				0xAA,		// LEDOUT0 (PWM)
				0xAA,		// LEDOUT1 (PWM)
				0xE2,		// SUBADR1
				0xE4,		// SUBADR2
				0xE8,		// SUBADR3
				0xE0,		// ALLLCALLADR
			};
	if( !writeFullConfig(pcaIn, fullDevConfig) ) return false;

	// setup our initial brightnesses
	memset(pcaIn->currBrightness, 0, sizeof(pcaIn->currBrightness));

	// enable our outputs
	cxa_gpio_setValue(pcaIn->gpio_outputEnable, 1);
	pcaIn->isInit = true;

	return true;
}


bool cxa_pca9624_setBrightnessForChannels(cxa_pca9624_t *const pcaIn, cxa_pca9624_channelEntry_t* chansEntriesIn, size_t numChansIn)
{
	cxa_assert(pcaIn);
	cxa_assert(chansEntriesIn);
	cxa_assert(numChansIn <= CXA_PCA9624_NUM_CHANNELS);

	// create a local copy of our brightnesses (in case we fail)
	uint8_t newBrightness[CXA_PCA9624_NUM_CHANNELS];
	memcpy(newBrightness, pcaIn->currBrightness, sizeof(newBrightness));

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
	memcpy(pcaIn->currBrightness, newBrightness, sizeof(pcaIn->currBrightness));
	return true;
}


// ******** local function implementations ********
static bool readFromRegister(cxa_pca9624_t *const pcaIn, register_t registerIn, uint8_t* valOut)
{
	cxa_assert(pcaIn);

	uint8_t ctrlBytes = registerIn;
	return cxa_i2cMaster_readBytes(pcaIn->i2c, pcaIn->address, &ctrlBytes, 1, valOut, 1);
}


static bool writeToRegister(cxa_pca9624_t *const pcaIn, register_t registerIn, uint8_t valIn)
{
	uint8_t ctrlBytes = registerIn;
	return cxa_i2cMaster_writeBytes(pcaIn->i2c, pcaIn->address, &ctrlBytes, 1, &valIn, 1);
}


static bool writeFullConfig(cxa_pca9624_t *const pcaIn, uint8_t* fullDevConfigIn)
{
	cxa_assert(pcaIn);

	uint8_t ctrlBytes = 0x80;
	return cxa_i2cMaster_writeBytes(pcaIn->i2c, pcaIn->address, &ctrlBytes, 1, fullDevConfigIn, 18);
}


static bool writeAllLedBrightnesses(cxa_pca9624_t *const pcaIn, uint8_t* brightnessesIn)
{
	cxa_assert(pcaIn);

	uint8_t ctrlBytes = 0xA0 | REG_PWM0;
	return cxa_i2cMaster_writeBytes(pcaIn->i2c, pcaIn->address, &ctrlBytes, 1, brightnessesIn, CXA_PCA9624_NUM_CHANNELS);
}
