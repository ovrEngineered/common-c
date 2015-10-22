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
#include "cxa_arduino_gpio.h"


// ******** includes ********
#include <Arduino.h>
#include <cxa_assert.h>


// ******** local macro definitions ********
#ifndef INPUT
	#define INPUT        0x0
#endif
#ifndef OUTPUT
	#define OUTPUT       0x1
#endif


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_arduino_gpio_init_input(cxa_arduino_gpio_t *const gpioIn, const uint16_t pinNumIn, const cxa_gpio_polarity_t polarityIn)
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


void cxa_arduino_gpio_init_output(cxa_arduino_gpio_t *const gpioIn, const uint16_t pinNumIn, const cxa_gpio_polarity_t polarityIn, const bool initValIn)
{
	cxa_assert(gpioIn);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );
	
	// save our references
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = polarityIn;
	gpioIn->lastSetVal_nonInverted = false;
	
	// set our initial value (before we set direction to avoid glitches)
	cxa_gpio_setValue(&gpioIn->super, initValIn);
	
	// now set our direction
	cxa_gpio_setDirection(&gpioIn->super, CXA_GPIO_DIR_OUTPUT);
}


void cxa_arduino_gpio_init_safe(cxa_arduino_gpio_t *const gpioIn, const uint16_t pinNumIn)
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
	cxa_arduino_gpio_t *const gpioIn = (cxa_arduino_gpio_t *const)superIn;

	// set our internal state
	if( dirIn == CXA_GPIO_DIR_UNKNOWN ) return;
	gpioIn->direction = dirIn;

	pinMode(gpioIn->pinNum, (dirIn == CXA_GPIO_DIR_INPUT) ? INPUT : OUTPUT);
}


cxa_gpio_direction_t cxa_gpio_getDirection(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_arduino_gpio_t *const gpioIn = (cxa_arduino_gpio_t *const)superIn;
	
	return gpioIn->direction;
}


void cxa_gpio_setPolarity(cxa_gpio_t *const superIn, const cxa_gpio_polarity_t polarityIn)
{
	cxa_assert(superIn);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );

	// get a pointer to our class
	cxa_arduino_gpio_t *const gpioIn = (cxa_arduino_gpio_t *const)superIn;

	// set our internal state
	gpioIn->polarity = polarityIn;
}


cxa_gpio_polarity_t cxa_gpio_getPolarity(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);
	
	// get a pointer to our class
	cxa_arduino_gpio_t *const gpioIn = (cxa_arduino_gpio_t *const)superIn;
	
	return gpioIn->polarity;
}


void cxa_gpio_setValue(cxa_gpio_t *const superIn, const bool valIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_arduino_gpio_t *const gpioIn = (cxa_arduino_gpio_t *const)superIn;

	gpioIn->lastSetVal_nonInverted = (gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED)? !valIn : valIn;
	if( cxa_gpio_getDirection(superIn) == CXA_GPIO_DIR_OUTPUT ) digitalWrite(gpioIn->pinNum, valIn);
}


bool cxa_gpio_getValue(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_arduino_gpio_t *const gpioIn = (cxa_arduino_gpio_t *const)superIn;

	bool retVal_nonInverted = (cxa_gpio_getDirection(superIn) == CXA_GPIO_DIR_INPUT) ? digitalRead(gpioIn->pinNum) : gpioIn->lastSetVal_nonInverted;
	return (gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !retVal_nonInverted : retVal_nonInverted;
}


void cxa_gpio_toggle(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	cxa_gpio_setValue(superIn, !cxa_gpio_getValue(superIn));
}


// ******** local function implementations ********

