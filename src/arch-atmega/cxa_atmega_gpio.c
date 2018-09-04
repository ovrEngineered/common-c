/**
 * Copyright 2018 opencxa.org
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
#include "cxa_atmega_gpio.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <avr/io.h>
#include <stdio.h>
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_setDirection(cxa_gpio_t *const superIn, const cxa_gpio_direction_t dirIn);
static cxa_gpio_direction_t scm_getDirection(cxa_gpio_t *const superIn);
static void scm_setPolarity(cxa_gpio_t *const superIn, const cxa_gpio_polarity_t polarityIn);
static cxa_gpio_polarity_t scm_getPolarity(cxa_gpio_t *const superIn);
static void scm_setValue(cxa_gpio_t *const superIn, const bool valIn);
static bool scm_getValue(cxa_gpio_t *const superIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_atmega_gpio_init_input(cxa_atmega_gpio_t *const gpioIn, const cxa_atmega_gpio_port_t portIn, const uint8_t pinNumIn, const cxa_gpio_polarity_t polarityIn)
{
	cxa_assert(gpioIn);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );

	// save our references
	gpioIn->port = portIn;
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = polarityIn;

	// initialize our super class
	cxa_gpio_init(&gpioIn->super, scm_setDirection, scm_getDirection, scm_setPolarity, scm_getPolarity, scm_setValue, scm_getValue, NULL);

	// set our initial direction
	cxa_gpio_setDirection(&gpioIn->super, CXA_GPIO_DIR_INPUT);
}


void cxa_atmega_gpio_init_output(cxa_atmega_gpio_t *const gpioIn, const cxa_atmega_gpio_port_t portIn, const uint8_t pinNumIn, const cxa_gpio_polarity_t polarityIn, const bool initValIn)
{
	cxa_assert(gpioIn);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );

	// save our references
	gpioIn->port = portIn;
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = polarityIn;

	// initialize our super class
	cxa_gpio_init(&gpioIn->super, scm_setDirection, scm_getDirection, scm_setPolarity, scm_getPolarity, scm_setValue, scm_getValue, NULL);

	// set our initial value (before we set direction to avoid glitches)
	cxa_gpio_setValue(&gpioIn->super, initValIn);

	// now set our direction
	cxa_gpio_setDirection(&gpioIn->super, CXA_GPIO_DIR_OUTPUT);

	cxa_gpio_setValue(&gpioIn->super, initValIn);
}


void cxa_atmega_gpio_init_safe(cxa_atmega_gpio_t *const gpioIn, const cxa_atmega_gpio_port_t portIn, const uint8_t pinNumIn)
{
	cxa_assert(gpioIn);
	cxa_assert(pinNumIn < 8);

	// save our references
	gpioIn->port = portIn;
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = CXA_GPIO_POLARITY_NONINVERTED;
	gpioIn->dir = CXA_GPIO_DIR_UNKNOWN;

	// initialze our super class
	cxa_gpio_init(&gpioIn->super, scm_setDirection, scm_getDirection, scm_setPolarity, scm_getPolarity, scm_setValue, scm_getValue, NULL);

	// don't set any value or direction...just leave everything as it is
}


// ******** local function implementations ********
static void scm_setDirection(cxa_gpio_t *const superIn, const cxa_gpio_direction_t dirIn)
{
	cxa_assert(superIn);
	cxa_assert( (dirIn == CXA_GPIO_DIR_INPUT) ||
				(dirIn == CXA_GPIO_DIR_OUTPUT) );

	// get a pointer to our class
	cxa_atmega_gpio_t *const gpioIn = (cxa_atmega_gpio_t *const)superIn;
	
	switch( gpioIn->port )
	{
		case CXA_ATM_GPIO_PORT_B:
			DDRB = (DDRB & ~(1 << gpioIn->pinNum)) | ((dirIn == CXA_GPIO_DIR_OUTPUT) << gpioIn->pinNum);
			break;

		case CXA_ATM_GPIO_PORT_C:
			DDRC = (DDRC & ~(1 << gpioIn->pinNum)) | ((dirIn == CXA_GPIO_DIR_OUTPUT) << gpioIn->pinNum);
			break;

		case CXA_ATM_GPIO_PORT_D:
			DDRD = (DDRD & ~(1 << gpioIn->pinNum)) | ((dirIn == CXA_GPIO_DIR_OUTPUT) << gpioIn->pinNum);
			break;
	}

	gpioIn->dir = dirIn;
}


static cxa_gpio_direction_t scm_getDirection(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_atmega_gpio_t *const gpioIn = (cxa_atmega_gpio_t *const)superIn;
	
	return gpioIn->dir;
}


static void scm_setPolarity(cxa_gpio_t *const superIn, const cxa_gpio_polarity_t polarityIn)
{
	cxa_assert(superIn);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );

	// get a pointer to our class
	cxa_atmega_gpio_t *const gpioIn = (cxa_atmega_gpio_t *const)superIn;
				
	gpioIn->polarity = polarityIn;
}


static cxa_gpio_polarity_t scm_getPolarity(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);
	
	// get a pointer to our class
	cxa_atmega_gpio_t *const gpioIn = (cxa_atmega_gpio_t *const)superIn;
	
	return gpioIn->polarity;
}


static void scm_setValue(cxa_gpio_t *const superIn, const bool valIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_atmega_gpio_t *const gpioIn = (cxa_atmega_gpio_t *const)superIn;

	bool tmpVal = (gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !valIn : valIn;

	switch( gpioIn->port )
	{
		case CXA_ATM_GPIO_PORT_B:
			PORTB = (PORTB & ~(1 << gpioIn->pinNum)) | (tmpVal << gpioIn->pinNum);
			break;

		case CXA_ATM_GPIO_PORT_C:
			PORTC = (PORTC & ~(1 << gpioIn->pinNum)) | (tmpVal << gpioIn->pinNum);
			break;

		case CXA_ATM_GPIO_PORT_D:
			PORTD = (PORTD & ~(1 << gpioIn->pinNum)) | (tmpVal << gpioIn->pinNum);
			break;
	}

	gpioIn->lastVal = tmpVal;
}


static bool scm_getValue(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_atmega_gpio_t *const gpioIn = (cxa_atmega_gpio_t *const)superIn;

	bool retVal = gpioIn->lastVal;

	if( gpioIn->dir == CXA_GPIO_DIR_INPUT )
	{
		switch( gpioIn->port )
		{
			case CXA_ATM_GPIO_PORT_B:
				retVal = (PINB & ~(1 << gpioIn->pinNum)) > 0;
				break;

			case CXA_ATM_GPIO_PORT_C:
				retVal = (PINC & ~(1 << gpioIn->pinNum)) > 0;
				break;

			case CXA_ATM_GPIO_PORT_D:
				retVal = (PIND & ~(1 << gpioIn->pinNum)) > 0;
				break;
		}
	}

	return (gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !retVal : retVal;
}

