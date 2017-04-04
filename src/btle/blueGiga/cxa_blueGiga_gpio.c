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
#include "cxa_blueGiga_gpio.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_blueGiga_btle_client.h>


#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void sendDirRegForPort(cxa_blueGiga_btle_client_t* btlecIn, uint8_t portNumIn);
static void sendAllOutputValsForPort(cxa_blueGiga_btle_client_t* btlecIn, uint8_t portNumIn);

static void scm_setDirection(cxa_gpio_t *const superIn, const cxa_gpio_direction_t dirIn);
static cxa_gpio_direction_t scm_getDirection(cxa_gpio_t *const superIn);
static void scm_setPolarity(cxa_gpio_t *const superIn, const cxa_gpio_polarity_t polarityIn);
static cxa_gpio_polarity_t scm_getPolarity(cxa_gpio_t *const superIn);
static void scm_setValue(cxa_gpio_t *const superIn, const bool valIn);
static bool scm_getValue(cxa_gpio_t *const superIn);

static void responseCb_ioPortDirConfig(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn);
static void responseCb_ioPortWriteValue(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn);


// ********  local variable declarations *********
static cxa_blueGiga_gpio_cb_onGpiosConfigured_t cb_onConfigured = NULL;
static bool isConfigingAllPorts = false;
static uint8_t configingPortNum = 0;


// ******** global function implementations ********
void cxa_blueGiga_gpio_init(cxa_blueGiga_gpio_t *const gpioIn, cxa_blueGiga_btle_client_t* btlecIn, uint8_t portNumIn, uint8_t chanNumIn)
{
	cxa_assert(gpioIn);
	cxa_assert(btlecIn);

	// if we made it here, we found a free GPIO
	gpioIn->isUsed = true;
	gpioIn->portNum = portNumIn;
	gpioIn->chanNum = chanNumIn;
	gpioIn->btlec = btlecIn;

	gpioIn->lastDirection = CXA_GPIO_DIR_INPUT;
	gpioIn->lastOutputVal = 0;
	gpioIn->polarity = CXA_GPIO_POLARITY_NONINVERTED;

	cxa_gpio_init(&gpioIn->super, scm_setDirection, scm_getDirection, scm_setPolarity, scm_getPolarity, scm_setValue, scm_getValue, NULL);
}


void cxa_blueGiga_configureGpiosForBlueGiga(cxa_blueGiga_btle_client_t* btlecIn, cxa_blueGiga_gpio_cb_onGpiosConfigured_t cbIn)
{
	cxa_assert(btlecIn);

	cb_onConfigured = cbIn;
	configingPortNum = 0;

	isConfigingAllPorts = true;
	sendDirRegForPort(btlecIn, configingPortNum);
}


// ******** local function implementations ********
static void sendDirRegForPort(cxa_blueGiga_btle_client_t* btlecIn, uint8_t portNumIn)
{
	cxa_assert(btlecIn);

	uint8_t dirReg = 0;

	for( size_t i = 0; i < sizeof(btlecIn->gpios)/sizeof(*btlecIn->gpios); i++ )
	{
		cxa_blueGiga_gpio_t* currGpio = (cxa_blueGiga_gpio_t*)&btlecIn->gpios[i];
		if( currGpio == NULL ) continue;

		if( currGpio->isUsed && (currGpio->portNum == portNumIn) && (currGpio->lastDirection == CXA_GPIO_DIR_OUTPUT) )
		{
			dirReg |= (1 << currGpio->chanNum);
		}
	}

	// get our message ready to send
	cxa_fixedByteBuffer_t fbb_params;
	uint8_t fbb_params_raw[2];
	cxa_fixedByteBuffer_initStd(&fbb_params, fbb_params_raw);
	cxa_fixedByteBuffer_append_uint8(&fbb_params, portNumIn);
	cxa_fixedByteBuffer_append_uint8(&fbb_params, dirReg);

	cxa_blueGiga_btle_client_sendCommand(btlecIn, CXA_BLUEGIGA_CLASSID_HARDWARE, CXA_BLUEGIGA_METHODID_HW_PORTCONFIGDIR, &fbb_params, responseCb_ioPortDirConfig, NULL);
}


static void sendAllOutputValsForPort(cxa_blueGiga_btle_client_t* btlecIn, uint8_t portNumIn)
{
	cxa_assert(btlecIn);

	uint8_t mask = 0;
	uint8_t val = 0;

	for( size_t i = 0; i < sizeof(btlecIn->gpios)/sizeof(*btlecIn->gpios); i++ )
	{
		cxa_blueGiga_gpio_t* currGpio = (cxa_blueGiga_gpio_t*)&btlecIn->gpios[i];
		if( currGpio == NULL ) continue;

		if( currGpio->isUsed && (currGpio->portNum == portNumIn) && (currGpio->lastDirection == CXA_GPIO_DIR_OUTPUT) )
		{
			mask |= (1 << currGpio->chanNum);

			uint8_t polVal = (currGpio->polarity == CXA_GPIO_POLARITY_INVERTED) ? !currGpio->lastOutputVal : currGpio->lastOutputVal;
			val |= (polVal << currGpio->chanNum);
		}
	}

	// get our message ready to send
	cxa_fixedByteBuffer_t fbb_params;
	uint8_t fbb_params_raw[3];
	cxa_fixedByteBuffer_initStd(&fbb_params, fbb_params_raw);
	cxa_fixedByteBuffer_append_uint8(&fbb_params, portNumIn);
	cxa_fixedByteBuffer_append_uint8(&fbb_params, mask);
	cxa_fixedByteBuffer_append_uint8(&fbb_params, val);

	cxa_blueGiga_btle_client_sendCommand(btlecIn, CXA_BLUEGIGA_CLASSID_HARDWARE, CXA_BLUEGIGA_METHODID_HW_PORTCONFIGDIR, &fbb_params, responseCb_ioPortWriteValue, NULL);
}


static void scm_setDirection(cxa_gpio_t *const superIn, const cxa_gpio_direction_t dirIn)
{
	cxa_blueGiga_gpio_t* gpioIn = (cxa_blueGiga_gpio_t*)superIn;
	cxa_assert(gpioIn);

	// save our direction
	gpioIn->lastDirection = dirIn;

	if( cxa_btle_client_isReady(&gpioIn->btlec->super) ) sendDirRegForPort(gpioIn->btlec, gpioIn->portNum);
}


static cxa_gpio_direction_t scm_getDirection(cxa_gpio_t *const superIn)
{
	cxa_blueGiga_gpio_t* gpioIn = (cxa_blueGiga_gpio_t*)superIn;
	cxa_assert(gpioIn);

	return gpioIn->lastDirection;
}


static void scm_setPolarity(cxa_gpio_t *const superIn, const cxa_gpio_polarity_t polarityIn)
{
	cxa_blueGiga_gpio_t* gpioIn = (cxa_blueGiga_gpio_t*)superIn;
	cxa_assert(gpioIn);
}


static cxa_gpio_polarity_t scm_getPolarity(cxa_gpio_t *const superIn)
{
	cxa_blueGiga_gpio_t* gpioIn = (cxa_blueGiga_gpio_t*)superIn;
	cxa_assert(gpioIn);

	return gpioIn->polarity;
}


static void scm_setValue(cxa_gpio_t *const superIn, const bool valIn)
{
	cxa_blueGiga_gpio_t* gpioIn = (cxa_blueGiga_gpio_t*)superIn;
	cxa_assert(gpioIn);

	// update our last output value
	uint8_t polVal = (gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !valIn : valIn;
	gpioIn->lastOutputVal = valIn;

	if( cxa_btle_client_isReady(&gpioIn->btlec->super) )
	{
		// get our message ready to send
		cxa_fixedByteBuffer_t fbb_params;
		uint8_t fbb_params_raw[3];
		cxa_fixedByteBuffer_initStd(&fbb_params, fbb_params_raw);
		cxa_fixedByteBuffer_append_uint8(&fbb_params, gpioIn->portNum);
		cxa_fixedByteBuffer_append_uint8(&fbb_params, (1 << gpioIn->chanNum));
		cxa_fixedByteBuffer_append_uint8(&fbb_params, (polVal << gpioIn->chanNum));

		cxa_blueGiga_btle_client_sendCommand(gpioIn->btlec, CXA_BLUEGIGA_CLASSID_HARDWARE, CXA_BLUEGIGA_METHODID_HW_PORTCONFIGDIR, &fbb_params, responseCb_ioPortWriteValue, NULL);
	}
}


static bool scm_getValue(cxa_gpio_t *const superIn)
{
	cxa_blueGiga_gpio_t* gpioIn = (cxa_blueGiga_gpio_t*)superIn;
	cxa_assert(gpioIn);

	return gpioIn->lastOutputVal;
}


static void responseCb_ioPortDirConfig(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn)
{
	cxa_assert(btlecIn);
	cxa_assert(payloadIn);

	// check our return value
	uint16_t response;
	if( !wasSuccessfulIn || !cxa_fixedByteBuffer_get_uint16LE(payloadIn, 0, response) || (response != 0) )
	{
		isConfigingAllPorts = false;
		cxa_logger_warn(&btlecIn->logger, "error setting port direction: %d", response);
		if( cb_onConfigured != NULL ) cb_onConfigured(btlecIn, false);
		return;
	}

	// if we made it here, we configured direction successfully...
	if( configingPortNum < 2 )
	{
		configingPortNum++;
		sendDirRegForPort(btlecIn, configingPortNum);
	}
	else
	{
		configingPortNum = 0;
		sendAllOutputValsForPort(btlecIn, configingPortNum);
	}
}


static void responseCb_ioPortWriteValue(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn)
{
	cxa_assert(btlecIn);
	cxa_assert(payloadIn);

	// check our return value
	uint16_t response;
	if( !wasSuccessfulIn || !cxa_fixedByteBuffer_get_uint16LE(payloadIn, 0, response) || (response != 0) )
	{
		isConfigingAllPorts = false;
		cxa_logger_warn(&btlecIn->logger, "error writing port value: %d", response);
		if( cb_onConfigured != NULL ) cb_onConfigured(btlecIn, false);
		return;
	}

	// if we made it here, we wrote output values successfully
	if( configingPortNum < 2 )
	{
		configingPortNum++;
		sendAllOutputValsForPort(btlecIn, configingPortNum);
	}
	else
	{
		// done configuring
		if( cb_onConfigured != NULL ) cb_onConfigured(btlecIn, true);
	}
}
