/**
 * Copyright 2013 opencxa.org
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
#include "cxa_xmega_gpio.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdio.h>
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_xmega_gpio_init_input(cxa_xmega_gpio_t *const gpioIn, PORT_t *const portIn, const uint8_t pinNumIn, const cxa_gpio_polarity_t polarityIn)
{
	cxa_assert(gpioIn);
	cxa_assert(portIn);
	cxa_assert(pinNumIn < 8);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );
	
	// save our references
	gpioIn->port = portIn;
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = polarityIn;
	
	// set our initial direction
	cxa_gpio_setDirection(&gpioIn->super, CXA_GPIO_DIR_INPUT);
}


void cxa_xmega_gpio_init_output(cxa_xmega_gpio_t *const gpioIn, PORT_t *const portIn, const uint8_t pinNumIn, const cxa_gpio_polarity_t polarityIn, const bool initValIn)
{
	cxa_assert(gpioIn);
	cxa_assert(portIn);
	cxa_assert(pinNumIn < 8);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );
	
	// save our references
	gpioIn->port = portIn;
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = polarityIn;
	
	// set our initial value (before we set direction to avoid glitches)
	cxa_gpio_setValue(&gpioIn->super, initValIn);
	
	// now set our direction
	cxa_gpio_setDirection(&gpioIn->super, CXA_GPIO_DIR_OUTPUT);
}


void cxa_xmega_gpio_init_safe(cxa_xmega_gpio_t *const gpioIn, PORT_t *const portIn, const uint8_t pinNumIn)
{
	cxa_assert(gpioIn);
	cxa_assert(portIn);
	cxa_assert(pinNumIn < 8);
	
	// save our references
	gpioIn->port = portIn;
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = CXA_GPIO_POLARITY_NONINVERTED;
	
	// don't set any value or direction...just leave everything as it is
}


void cxa_gpio_setDirection(cxa_gpio_t *const superIn, const cxa_gpio_direction_t dirIn)
{
	cxa_assert(superIn);
	cxa_assert( (dirIn == CXA_GPIO_DIR_INPUT) ||
				(dirIn == CXA_GPIO_DIR_OUTPUT) );

	// get a pointer to our class
	cxa_xmega_gpio_t *const gpioIn = (cxa_xmega_gpio_t *const)superIn;
	
	if( dirIn == CXA_GPIO_DIR_OUTPUT ) gpioIn->port->DIR |= (1 << gpioIn->pinNum);
	else gpioIn->port->DIR &= ~(1 << gpioIn->pinNum);
}


cxa_gpio_direction_t cxa_gpio_getDirection(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_xmega_gpio_t *const gpioIn = (cxa_xmega_gpio_t *const)superIn;
	
	return ((gpioIn->port->DIR & (1 << gpioIn->pinNum)) ? CXA_GPIO_DIR_OUTPUT : CXA_GPIO_DIR_INPUT);
}


void cxa_gpio_setPolarity(cxa_gpio_t *const superIn, const cxa_gpio_polarity_t polarityIn)
{
	cxa_assert(superIn);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );

	// get a pointer to our class
	cxa_xmega_gpio_t *const gpioIn = (cxa_xmega_gpio_t *const)superIn;
				
	gpioIn->polarity = polarityIn;
}


cxa_gpio_polarity_t cxa_gpio_getPolarity(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);
	
	// get a pointer to our class
	cxa_xmega_gpio_t *const gpioIn = (cxa_xmega_gpio_t *const)superIn;
	
	return gpioIn->polarity;
}


void cxa_gpio_setValue(cxa_gpio_t *const superIn, const bool valIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_xmega_gpio_t *const gpioIn = (cxa_xmega_gpio_t *const)superIn;

	gpioIn->port->OUT = (gpioIn->port->OUT & ~(1 << gpioIn->pinNum)) |
			(((gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !valIn : valIn) << gpioIn->pinNum);
}


bool cxa_gpio_getValue(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_xmega_gpio_t *const gpioIn = (cxa_xmega_gpio_t *const)superIn;

	bool retVal = (cxa_gpio_getDirection(superIn) == CXA_GPIO_DIR_OUTPUT) ?
		((gpioIn->port->OUT & (1 << gpioIn->pinNum)) >> gpioIn->pinNum) :
		((gpioIn->port->IN & (1 << gpioIn->pinNum)) >> gpioIn->pinNum);
		
	return (gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !retVal : retVal;
}


void cxa_gpio_toggle(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_xmega_gpio_t *const gpioIn = (cxa_xmega_gpio_t *const)superIn;
	
	gpioIn->port->OUTTGL = (1 << gpioIn->pinNum);
}


// ******** local function implementations ********

