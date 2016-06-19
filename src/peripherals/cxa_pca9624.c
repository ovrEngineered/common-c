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
static uint8_t getLedChanNum(cxa_led_pca9624_t *ledIn);
static uint8_t changeLedOutValue(uint8_t ledChanNum, ledOut_state_t stateIn, uint8_t currValIn);
static bool writeFullConfig(cxa_pca9624_t *const pcaIn, uint8_t* fullDevConfig);

static void scm_turnOn(cxa_led_t *const superIn);
static void scm_turnOff(cxa_led_t *const superIn);
static void scm_setBrightness(cxa_led_t *const superIn, uint8_t brightnessIn);


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
				0x00,		// LEDOUT0
				0x00,		// LEDOUT1
				0xE2,		// SUBADR1
				0xE4,		// SUBADR2
				0xE8,		// SUBADR3
				0xE0,		// ALLLCALLADR
			};
	if( !writeFullConfig(pcaIn, fullDevConfig) ) return false;

	// setup our LEDS
	for( int i = 0; i < (sizeof(pcaIn->leds)/sizeof(*pcaIn->leds)); i++ )
	{
		pcaIn->leds[i].pca = pcaIn;
		cxa_led_init(&pcaIn->leds[i].super, scm_turnOn, scm_turnOff, NULL, scm_setBrightness);
		// all LEDs default to off
	}

	// enable our outputs
	cxa_gpio_setValue(pcaIn->gpio_outputEnable, 1);
	pcaIn->isInit = true;

	return true;
}


cxa_led_t* cxa_pca9624_getLed(cxa_pca9624_t *const pcaIn, uint8_t chanNumIn)
{
	cxa_assert(pcaIn);
	cxa_assert(chanNumIn < (sizeof(pcaIn->leds)/sizeof(*pcaIn->leds)));

	return &pcaIn->leds[chanNumIn].super;
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


static uint8_t getLedChanNum(cxa_led_pca9624_t *ledIn)
{
	cxa_assert(ledIn);
	cxa_assert(ledIn->pca);

	for( size_t i = 0; i < (sizeof(ledIn->pca->leds)/sizeof(*ledIn->pca->leds)); i++ )
	{
		if( ledIn == &ledIn->pca->leds[i] ) return i;
	}

	// if we made it here, something is seriously messed up
	cxa_assert(0);
	return 0;
}


static uint8_t changeLedOutValue(uint8_t ledChanNum, ledOut_state_t stateIn, uint8_t currValIn)
{
	switch( ledChanNum )
	{
		case 0:
		case 4:
			currValIn = (currValIn & ~(0x03 << 0)) | ((stateIn & 0x03) << 0);
			break;

		case 1:
		case 5:
			currValIn = (currValIn & ~(0x03 << 2)) | ((stateIn & 0x03) << 2);
			break;

		case 2:
		case 6:
			currValIn = (currValIn & ~(0x03 << 4)) | ((stateIn & 0x03) << 4);
			break;

		case 3:
		case 7:
			currValIn = (currValIn & ~(0x03 << 6)) | ((stateIn & 0x03) << 6);
			break;
	}

	return currValIn;
}


static void scm_turnOn(cxa_led_t *const superIn)
{
	cxa_led_pca9624_t* ledIn = (cxa_led_pca9624_t*)superIn;
	cxa_assert(ledIn);

	if( !ledIn->pca->isInit ) return;

	// get our current register value
	uint8_t chanNum = getLedChanNum(ledIn);
	register_t reg = (chanNum <= 3) ? REG_LEDOUT0 : REG_LEDOUT1;
	uint8_t regVal;
	if( !readFromRegister(ledIn->pca, reg, &regVal) ) return;

	// modify it as needed
	regVal = changeLedOutValue(chanNum, LEDOUT_ON, regVal);

	writeToRegister(ledIn->pca, reg, regVal);
}


static void scm_turnOff(cxa_led_t *const superIn)
{
	cxa_led_pca9624_t* ledIn = (cxa_led_pca9624_t*)superIn;
	cxa_assert(ledIn);

	if( !ledIn->pca->isInit ) return;

	// get our current register value
	uint8_t chanNum = getLedChanNum(ledIn);
	register_t reg = (chanNum <= 3) ? REG_LEDOUT0 : REG_LEDOUT1;
	uint8_t regVal;
	if( !readFromRegister(ledIn->pca, reg, &regVal) ) return;

	// modify it as needed
	regVal = changeLedOutValue(chanNum, LEDOUT_OFF, regVal);

	writeToRegister(ledIn->pca, reg, regVal);
}


static void scm_setBrightness(cxa_led_t *const superIn, uint8_t brightnessIn)
{
	cxa_led_pca9624_t* ledIn = (cxa_led_pca9624_t*)superIn;
	cxa_assert(ledIn);

	if( !ledIn->pca->isInit ) return;
}
