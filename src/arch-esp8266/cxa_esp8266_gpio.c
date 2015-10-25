/**
 * Copyright 2015 opencxa.org
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
 *
 * @author Christopher Armenio
 */
#include "cxa_esp8266_gpio.h"


// ******** includes ********
#include <eagle_soc.h>

#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********
typedef struct
{
	uint8_t pinNum;
	uint32_t muxVal;
	uint8_t func;
}pinRegMapEntry_t;


// ******** local function prototypes ********
static pinRegMapEntry_t* getPinRegMapEntry_forPinNum(uint8_t pinNumIn);


// ********  local variable declarations *********
static pinRegMapEntry_t pinRegMap[] =
{
	{ 0, PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0},
	{ 1, PERIPHS_IO_MUX_U0TXD_U, FUNC_GPIO1},
	{ 2, PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2},
	{ 3, PERIPHS_IO_MUX_U0RXD_U, FUNC_GPIO3},
	{ 4, PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4},
	{ 5, PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5},
	{ 9, PERIPHS_IO_MUX_SD_DATA2_U, FUNC_GPIO9},
	{10, PERIPHS_IO_MUX_SD_DATA3_U, FUNC_GPIO10},
	{12, PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12},
	{13, PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13},
	{14, PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14},
	{15, PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15},
};


// ******** global function implementations ********
void cxa_esp8266_gpio_init_input(cxa_esp8266_gpio_t *const gpioIn, const uint8_t pinNumIn, const cxa_gpio_polarity_t polarityIn)
{
	cxa_assert(gpioIn);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );
	
	// save our references
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = polarityIn;
	gpioIn->lastSetVal_nonInverted = false;
	
	// set our initial direction
	cxa_gpio_setDirection(&gpioIn->super, CXA_GPIO_DIR_INPUT);
}


void cxa_esp8266_gpio_init_output(cxa_esp8266_gpio_t *const gpioIn, const uint8_t pinNumIn, const cxa_gpio_polarity_t polarityIn, const bool initValIn)
{
	cxa_assert(gpioIn);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );
	
	// save our references
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = polarityIn;
	gpioIn->lastSetVal_nonInverted = false;
	
	// configure the pin for GPIO
	pinRegMapEntry_t* currEntry = getPinRegMapEntry_forPinNum(gpioIn->pinNum);
	cxa_assert(currEntry);
	PIN_FUNC_SELECT((currEntry->muxVal), (currEntry->func));

	// set our initial value (before we set direction to avoid glitches)
	cxa_gpio_setValue(&gpioIn->super, initValIn);
	
	// now set our direction
	cxa_gpio_setDirection(&gpioIn->super, CXA_GPIO_DIR_OUTPUT);
}


void cxa_esp8266_gpio_init_safe(cxa_esp8266_gpio_t *const gpioIn, const uint8_t pinNumIn)
{
	cxa_assert(gpioIn);
	
	// save our references
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = CXA_GPIO_POLARITY_NONINVERTED;
	gpioIn->lastSetVal_nonInverted = false;
	
	// don't set any value or direction...just leave everything as it is
}


void cxa_gpio_setDirection(cxa_gpio_t *const superIn, const cxa_gpio_direction_t dirIn)
{
	cxa_assert(superIn);
	cxa_assert( (dirIn == CXA_GPIO_DIR_INPUT) ||
				(dirIn == CXA_GPIO_DIR_OUTPUT) );

	// get a pointer to our class
	cxa_esp8266_gpio_t *const gpioIn = (cxa_esp8266_gpio_t *const)superIn;

	// configure out register
	GPIO_REG_WRITE(((dirIn == CXA_GPIO_DIR_OUTPUT) ? GPIO_ENABLE_W1TS_ADDRESS : GPIO_ENABLE_W1TC_ADDRESS), (1<<gpioIn->pinNum));
}


cxa_gpio_direction_t cxa_gpio_getDirection(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_esp8266_gpio_t *const gpioIn = (cxa_esp8266_gpio_t *const)superIn;
	
	return ((GPIO_REG_READ(GPIO_ENABLE_ADDRESS) >> gpioIn->pinNum) & 1) ? CXA_GPIO_DIR_INPUT : CXA_GPIO_DIR_OUTPUT;
}


void cxa_gpio_setPolarity(cxa_gpio_t *const superIn, const cxa_gpio_polarity_t polarityIn)
{
	cxa_assert(superIn);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );

	// get a pointer to our class
	cxa_esp8266_gpio_t *const gpioIn = (cxa_esp8266_gpio_t *const)superIn;

	// set our internal state
	gpioIn->polarity = polarityIn;
}


cxa_gpio_polarity_t cxa_gpio_getPolarity(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);
	
	// get a pointer to our class
	cxa_esp8266_gpio_t *const gpioIn = (cxa_esp8266_gpio_t *const)superIn;
	
	return gpioIn->polarity;
}


void cxa_gpio_setValue(cxa_gpio_t *const superIn, const bool valIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_esp8266_gpio_t *const gpioIn = (cxa_esp8266_gpio_t *const)superIn;

	gpioIn->lastSetVal_nonInverted = (gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !valIn : valIn;

	GPIO_REG_WRITE(((gpioIn->lastSetVal_nonInverted ? GPIO_OUT_W1TS_ADDRESS : GPIO_OUT_W1TC_ADDRESS)), (1 << gpioIn->pinNum));
}


bool cxa_gpio_getValue(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_esp8266_gpio_t *const gpioIn = (cxa_esp8266_gpio_t *const)superIn;

	bool retVal_nonInverted = (cxa_gpio_getDirection(superIn) == CXA_GPIO_DIR_INPUT) ? ((GPIO_REG_READ(GPIO_IN_ADDRESS) >> gpioIn->pinNum) & 1) : gpioIn->lastSetVal_nonInverted;
	return (gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !retVal_nonInverted : retVal_nonInverted;
}


void cxa_gpio_toggle(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	cxa_gpio_setValue(superIn, !cxa_gpio_getValue(superIn));
}


// ******** local function implementations ********
static pinRegMapEntry_t* getPinRegMapEntry_forPinNum(uint8_t pinNumIn)
{
	for( size_t i = 0; i < (sizeof(pinRegMap)/sizeof(*pinRegMap)); i++ )
	{
		pinRegMapEntry_t* currEntry = &pinRegMap[i];

		if( currEntry->pinNum == pinNumIn ) return currEntry;
	}

	return NULL;
}

