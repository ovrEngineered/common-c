/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_pca9555.h"


// ******** includes ********
#include <stdio.h>
#include <cxa_assert.h>


#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********
typedef enum
{
	REG_INPUT0 = 0x00,
	REG_INPUT1 = 0x01,
	REG_OUTPUT0 = 0x02,
	REG_OUTPUT1 = 0x03,
	REG_POL0 = 0x04,
	REG_POL1 = 0x05,
	REG_CFG0 = 0x06,
	REG_CFG1 = 0x07
}pca_register_t;


// ******** local function prototypes ********
static bool readFromRegister(cxa_pca9555_t *const pcaIn, pca_register_t registerIn, uint8_t* valOut);
static bool writeToRegister(cxa_pca9555_t *const pcaIn, pca_register_t registerIn, uint8_t valIn);

static void getGpioPortAndChannelNum(cxa_gpio_pca9555_t *gpioIn, uint8_t* portOut, uint8_t* chanNumOut);
static uint8_t getDirValueForPort(cxa_pca9555_t *pcaIn, uint8_t portNumIn);
static uint8_t getOutputValueForPort(cxa_pca9555_t *pcaIn, uint8_t portNumIn);

static void scm_setDirection(cxa_gpio_t *const superIn, const cxa_gpio_direction_t dirIn);
static cxa_gpio_direction_t scm_getDirection(cxa_gpio_t *const superIn);
static void scm_setPolarity(cxa_gpio_t *const superIn, const cxa_gpio_polarity_t polarityIn);
static cxa_gpio_polarity_t scm_getPolarity(cxa_gpio_t *const superIn);
static void scm_setValue(cxa_gpio_t *const superIn, const bool valIn);
static bool scm_getValue(cxa_gpio_t *const superIn);


// ********  local variable declarations *********


// ******** global function implementations ********
bool cxa_pca9555_init(cxa_pca9555_t *const pcaIn, cxa_i2cMaster_t *const i2cIn, uint8_t addressIn)
{
	cxa_assert(pcaIn);
	cxa_assert(i2cIn);

	// save our references
	pcaIn->address = addressIn;
	pcaIn->i2c = i2cIn;

	// read our configuration regs
	uint8_t cfg0, cfg1, out0, out1;
	if( !readFromRegister(pcaIn, REG_CFG0, &cfg0) ||
		!readFromRegister(pcaIn, REG_CFG1, &cfg1) ||
		!readFromRegister(pcaIn, REG_OUTPUT0, &out0) ||
		!readFromRegister(pcaIn, REG_OUTPUT1, &out1) ) return false;

	// setup our GPIOs
	for( int i = 0; i < (sizeof(pcaIn->gpios_port0)/sizeof(*pcaIn->gpios_port0)); i++ )
	{
		pcaIn->gpios_port0[i].pca = pcaIn;
		pcaIn->gpios_port0[i].polarity = CXA_GPIO_POLARITY_NONINVERTED;
		pcaIn->gpios_port0[i].lastDirection = ((cfg0 >> i) & 0x01) ? CXA_GPIO_DIR_INPUT : CXA_GPIO_DIR_OUTPUT;
		pcaIn->gpios_port0[i].lastOutputVal = ((out0 >> i) & 0x01);

		// initialize our super class
		cxa_gpio_init(&pcaIn->gpios_port0[i].super, scm_setDirection, scm_getDirection, scm_setPolarity, scm_getPolarity, scm_setValue, scm_getValue, NULL);
	}
	for( int i = 0; i < (sizeof(pcaIn->gpios_port1)/sizeof(*pcaIn->gpios_port1)); i++ )
	{
		pcaIn->gpios_port1[i].pca = pcaIn;
		pcaIn->gpios_port1[i].polarity = CXA_GPIO_POLARITY_NONINVERTED;
		pcaIn->gpios_port0[i].lastDirection = ((cfg1 >> i) & 0x01) ? CXA_GPIO_DIR_INPUT : CXA_GPIO_DIR_OUTPUT;
		pcaIn->gpios_port1[i].lastOutputVal = ((out1 >> i) & 0x01);

		// initialize our super class
		cxa_gpio_init(&pcaIn->gpios_port1[i].super, scm_setDirection, scm_getDirection, scm_setPolarity, scm_getPolarity, scm_setValue, scm_getValue, NULL);
	}

	return true;
}


cxa_gpio_t* cxa_pca9555_getGpio(cxa_pca9555_t *const pcaIn, uint8_t portNumIn, uint8_t chanNumIn)
{
	cxa_assert(pcaIn);
	cxa_assert((portNumIn == 0) || (portNumIn == 1));
	if( portNumIn == 0 ) cxa_assert(chanNumIn < (sizeof(pcaIn->gpios_port0)/sizeof(*pcaIn->gpios_port0)));
	if( portNumIn == 1 ) cxa_assert(chanNumIn < (sizeof(pcaIn->gpios_port1)/sizeof(*pcaIn->gpios_port1)));

	return (portNumIn == 0) ? &pcaIn->gpios_port0[chanNumIn].super : &pcaIn->gpios_port1[chanNumIn].super;
}


// ******** local function implementations ********
static bool readFromRegister(cxa_pca9555_t *const pcaIn, pca_register_t registerIn, uint8_t* valOut)
{
	cxa_assert(pcaIn);

//	uint8_t ctrlBytes = registerIn;
#warning fix this
	return false;
//	return cxa_i2cMaster_readBytes(pcaIn->i2c, pcaIn->address, &ctrlBytes, 1, valOut, 1);
}


static bool writeToRegister(cxa_pca9555_t *const pcaIn, pca_register_t registerIn, uint8_t valIn)
{
//	uint8_t ctrlBytes = registerIn;
#warning fix this
	return false;
//	return cxa_i2cMaster_writeBytes(pcaIn->i2c, pcaIn->address, &ctrlBytes, 1, &valIn, 1);
}


static void getGpioPortAndChannelNum(cxa_gpio_pca9555_t *gpioIn, uint8_t* portOut, uint8_t* chanNumOut)
{
	cxa_assert(gpioIn);
	cxa_assert(gpioIn->pca);

	// check the port 0 first
	for( size_t i = 0; i < (sizeof(gpioIn->pca->gpios_port0)/sizeof(*gpioIn->pca->gpios_port0)); i++ )
	{
		if( gpioIn == &gpioIn->pca->gpios_port0[i] )
		{
			if( portOut != NULL ) *portOut = 0;
			if( chanNumOut != NULL ) *chanNumOut = i;
			return;
		}
	}

	// now check port 1
	for( size_t i = 0; i < (sizeof(gpioIn->pca->gpios_port1)/sizeof(*gpioIn->pca->gpios_port1)); i++ )
	{
		if( gpioIn == &gpioIn->pca->gpios_port1[i] )
		{
			if( portOut != NULL ) *portOut = 1;
			if( chanNumOut != NULL ) *chanNumOut = i;
			return;
		}
	}

	// if we made it here, something is seriously messed up
	cxa_assert(0);
}


static uint8_t getDirValueForPort(cxa_pca9555_t *pcaIn, uint8_t portNumIn)
{
	cxa_assert(pcaIn);
	cxa_assert((portNumIn == 0) || (portNumIn == 1));

	uint8_t retVal = 0;
	if( portNumIn == 0 )
	{
		for( size_t i = 0; i < (sizeof(pcaIn->gpios_port0)/sizeof(*pcaIn->gpios_port0)); i++ )
		{
			cxa_gpio_pca9555_t* currGpio = &pcaIn->gpios_port0[i];
			retVal |= ((currGpio->lastDirection == CXA_GPIO_DIR_INPUT) ? 1 : 0) << i;
		}
	}
	else
	{
		for( size_t i = 0; i < (sizeof(pcaIn->gpios_port1)/sizeof(*pcaIn->gpios_port1)); i++ )
		{
			cxa_gpio_pca9555_t* currGpio = &pcaIn->gpios_port1[i];
			retVal |= ((currGpio->lastDirection == CXA_GPIO_DIR_INPUT) ? 1 : 0) << i;
		}
	}
	return retVal;
}


static uint8_t getOutputValueForPort(cxa_pca9555_t *pcaIn, uint8_t portNumIn)
{
	cxa_assert(pcaIn);
	cxa_assert((portNumIn == 0) || (portNumIn == 1));

	uint8_t retVal = 0;
	if( portNumIn == 0 )
	{
		for( size_t i = 0; i < (sizeof(pcaIn->gpios_port0)/sizeof(*pcaIn->gpios_port0)); i++ )
		{
			cxa_gpio_pca9555_t* currGpio = &pcaIn->gpios_port0[i];
			retVal |= currGpio->lastOutputVal << i;
		}
	}
	else
	{
		for( size_t i = 0; i < (sizeof(pcaIn->gpios_port1)/sizeof(*pcaIn->gpios_port1)); i++ )
		{
			cxa_gpio_pca9555_t* currGpio = &pcaIn->gpios_port1[i];
			retVal |= currGpio->lastOutputVal << i;
		}
	}
	return retVal;
}


static void scm_setDirection(cxa_gpio_t *const superIn, const cxa_gpio_direction_t dirIn)
{
	cxa_gpio_pca9555_t* gpioIn = (cxa_gpio_pca9555_t*)superIn;
	cxa_assert(gpioIn);

	uint8_t portNum, chanNum;
	getGpioPortAndChannelNum(gpioIn, &portNum, &chanNum);
	pca_register_t reg = (portNum == 0) ? REG_CFG0 : REG_CFG1;

	uint8_t dirVal = getDirValueForPort(gpioIn->pca, portNum);
	dirVal = (dirVal & ~(1 << chanNum)) | (((dirIn == CXA_GPIO_DIR_INPUT) ? 1 : 0) << chanNum);

	if( writeToRegister(gpioIn->pca, reg, dirVal) ) gpioIn->lastDirection = dirIn;
}


static cxa_gpio_direction_t scm_getDirection(cxa_gpio_t *const superIn)
{
	cxa_gpio_pca9555_t* gpioIn = (cxa_gpio_pca9555_t*)superIn;
	cxa_assert(gpioIn);

	uint8_t portNum, chanNum;
	getGpioPortAndChannelNum(gpioIn, &portNum, &chanNum);
	pca_register_t reg = (portNum == 0) ? REG_CFG0 : REG_CFG1;

	uint8_t dirVal;
	cxa_gpio_direction_t retVal = CXA_GPIO_DIR_UNKNOWN;
	if( readFromRegister(gpioIn->pca, reg, &dirVal) )
	{
		retVal = ((dirVal >> chanNum) & 0x01) ? CXA_GPIO_DIR_INPUT : CXA_GPIO_DIR_OUTPUT;
	}
	gpioIn->lastDirection = retVal;
	return retVal;
}


static void scm_setPolarity(cxa_gpio_t *const superIn, const cxa_gpio_polarity_t polarityIn)
{
	cxa_gpio_pca9555_t* gpioIn = (cxa_gpio_pca9555_t*)superIn;
	cxa_assert(gpioIn);

	gpioIn->polarity = polarityIn;
}


static cxa_gpio_polarity_t scm_getPolarity(cxa_gpio_t *const superIn)
{
	cxa_gpio_pca9555_t* gpioIn = (cxa_gpio_pca9555_t*)superIn;
	cxa_assert(gpioIn);

	return gpioIn->polarity;
}


static void scm_setValue(cxa_gpio_t *const superIn, const bool valIn)
{
	cxa_gpio_pca9555_t* gpioIn = (cxa_gpio_pca9555_t*)superIn;
	cxa_assert(gpioIn);

	uint8_t portNum, chanNum;
	getGpioPortAndChannelNum(gpioIn, &portNum, &chanNum);
	pca_register_t reg = (portNum == 0) ? REG_OUTPUT0 : REG_OUTPUT1;

	uint8_t outputVal = getOutputValueForPort(gpioIn->pca, portNum);
	uint8_t polVal = (gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !valIn : valIn;
	outputVal = (outputVal & ~(1 << chanNum)) | (polVal << chanNum);

	if( writeToRegister(gpioIn->pca, reg, outputVal) ) gpioIn->lastOutputVal = valIn;
}


static bool scm_getValue(cxa_gpio_t *const superIn)
{
	cxa_gpio_pca9555_t* gpioIn = (cxa_gpio_pca9555_t*)superIn;
	cxa_assert(gpioIn);

	// shortcut reading from the device if we can
	if( (gpioIn->lastDirection == CXA_GPIO_DIR_OUTPUT) ) return gpioIn->lastOutputVal;

	// couldn't shortcut...read from the device
	uint8_t portNum, chanNum;
	getGpioPortAndChannelNum(gpioIn, &portNum, &chanNum);
	pca_register_t reg = (portNum == 0) ? REG_OUTPUT0 : REG_OUTPUT1;

	uint8_t inputVal;
	if( !readFromRegister(gpioIn->pca, reg, &inputVal) ) return 0;
	return ((inputVal >> chanNum) & 0x01);
}
