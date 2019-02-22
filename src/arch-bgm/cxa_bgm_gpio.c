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
#include "cxa_bgm_gpio.h"


// ******** includes ********
#include <stdio.h>
#include <cxa_assert.h>

#include "em_cmu.h"


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void initSystem(void);
static void scm_setDirection(cxa_gpio_t *const superIn, const cxa_gpio_direction_t dirIn);
static cxa_gpio_direction_t scm_getDirection(cxa_gpio_t *const superIn);
static void scm_setPolarity(cxa_gpio_t *const superIn, const cxa_gpio_polarity_t polarityIn);
static cxa_gpio_polarity_t scm_getPolarity(cxa_gpio_t *const superIn);
static void scm_setValue(cxa_gpio_t *const superIn, const bool valIn);
static bool scm_getValue(cxa_gpio_t *const superIn);


// ********  local variable declarations *********
static bool isSystemInit = false;


// ******** global function implementations ********
void cxa_bgm_gpio_init_input(cxa_bgm_gpio_t *const gpioIn,
							 const GPIO_Port_TypeDef portNumIn, const unsigned int pinNumIn,
							 const cxa_gpio_polarity_t polarityIn)
{
	cxa_assert(gpioIn);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );

	// make sure our system is initialized
	if( !isSystemInit ) initSystem();

	// save our references
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = polarityIn;
	gpioIn->port = portNumIn;
	gpioIn->pinNum = pinNumIn;

	// initialize our super class
	cxa_gpio_init(&gpioIn->super, scm_setDirection, scm_getDirection, scm_setPolarity, scm_getPolarity, scm_setValue, scm_getValue, NULL);

	// set our initial direction
	cxa_gpio_setDirection(&gpioIn->super, CXA_GPIO_DIR_INPUT);
}


void cxa_bgm_gpio_init_output(cxa_bgm_gpio_t *const gpioIn,
							  const GPIO_Port_TypeDef portNumIn, const unsigned int pinNumIn,
							  const cxa_gpio_polarity_t polarityIn, const bool initValIn)
{
	cxa_assert(gpioIn);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );

	// make sure our system is initialized
	if( !isSystemInit ) initSystem();

	// save our references
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = polarityIn;
	gpioIn->port = portNumIn;
	gpioIn->pinNum = pinNumIn;

	// initialize our super class
	cxa_gpio_init(&gpioIn->super, scm_setDirection, scm_getDirection, scm_setPolarity, scm_getPolarity, scm_setValue, scm_getValue, NULL);

	// set our initial value (before we set direction to avoid glitches)
	gpioIn->lastVal = initValIn;

	// now set our direction
	cxa_gpio_setDirection(&gpioIn->super, CXA_GPIO_DIR_OUTPUT);
}


void cxa_bgm_gpio_init_safe(cxa_bgm_gpio_t *const gpioIn,
							const GPIO_Port_TypeDef portNumIn, const unsigned int pinNumIn)
{
	cxa_assert(gpioIn);
	cxa_assert(pinNumIn < 8);

	// make sure our system is initialized
	if( !isSystemInit ) initSystem();

	// save our references
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = CXA_GPIO_POLARITY_NONINVERTED;
	gpioIn->port = portNumIn;
	gpioIn->pinNum = pinNumIn;
	gpioIn->dir = CXA_GPIO_DIR_UNKNOWN;

	// initialze our super class
	cxa_gpio_init(&gpioIn->super, scm_setDirection, scm_getDirection, scm_setPolarity, scm_getPolarity, scm_setValue, scm_getValue, NULL);

	// don't set any value or direction...just leave everything as it is
}


// ******** local function implementations ********
static void initSystem(void)
{
	CMU_ClockEnable(cmuClock_GPIO, true);
	isSystemInit = true;
}


static void scm_setDirection(cxa_gpio_t *const superIn, const cxa_gpio_direction_t dirIn)
{
	cxa_assert(superIn);
	cxa_assert( (dirIn == CXA_GPIO_DIR_INPUT) ||
				(dirIn == CXA_GPIO_DIR_OUTPUT) );

	// get a pointer to our class
	cxa_bgm_gpio_t *const gpioIn = (cxa_bgm_gpio_t *const)superIn;
	
	if( dirIn == CXA_GPIO_DIR_INPUT )
	{
		GPIO_PinModeSet(gpioIn->port, gpioIn->pinNum, gpioModeInput, 0);
	}
	else if( dirIn == CXA_GPIO_DIR_OUTPUT )
	{
		bool setVal = (gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !gpioIn->lastVal : gpioIn->lastVal;
		GPIO_PinModeSet(gpioIn->port, gpioIn->pinNum, gpioModePushPull, setVal);
	}
	gpioIn->dir = dirIn;
}


static cxa_gpio_direction_t scm_getDirection(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_bgm_gpio_t *const gpioIn = (cxa_bgm_gpio_t *const)superIn;
	
	return gpioIn->dir;
}


static void scm_setPolarity(cxa_gpio_t *const superIn, const cxa_gpio_polarity_t polarityIn)
{
	cxa_assert(superIn);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );

	// get a pointer to our class
	cxa_bgm_gpio_t *const gpioIn = (cxa_bgm_gpio_t *const)superIn;
				
	gpioIn->polarity = polarityIn;
}


static cxa_gpio_polarity_t scm_getPolarity(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);
	
	// get a pointer to our class
	cxa_bgm_gpio_t *const gpioIn = (cxa_bgm_gpio_t *const)superIn;
	
	return gpioIn->polarity;
}


static void scm_setValue(cxa_gpio_t *const superIn, const bool valIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_bgm_gpio_t *const gpioIn = (cxa_bgm_gpio_t *const)superIn;

	bool setVal = (gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !valIn : valIn;
	if( setVal ) GPIO_PinOutSet(gpioIn->port, gpioIn->pinNum);
	else GPIO_PinOutClear(gpioIn->port, gpioIn->pinNum);

	gpioIn->lastVal = valIn;
}


static bool scm_getValue(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_bgm_gpio_t *const gpioIn = (cxa_bgm_gpio_t *const)superIn;

	bool retVal = (gpioIn->dir == CXA_GPIO_DIR_INPUT) ? GPIO_PinInGet(gpioIn->port, gpioIn->pinNum) : gpioIn->lastVal;
	return (gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !retVal : retVal;
}

