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
#include "cxa_ble112_gpio.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdio.h>
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void setSelToGpio(cxa_ble112_gpio_t *const gpioIn);

static void scm_setDirection(cxa_gpio_t *const superIn, const cxa_gpio_direction_t dirIn);
static cxa_gpio_direction_t scm_getDirection(cxa_gpio_t *const superIn);
static void scm_setPolarity(cxa_gpio_t *const superIn, const cxa_gpio_polarity_t polarityIn);
static cxa_gpio_polarity_t scm_getPolarity(cxa_gpio_t *const superIn);
static void scm_setValue(cxa_gpio_t *const superIn, const bool valIn);
static bool scm_getValue(cxa_gpio_t *const superIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_ble112_gpio_init_input(cxa_ble112_gpio_t *const gpioIn, cxa_ble112_gpio_port_t portIn, const uint8_t pinNumIn, const cxa_gpio_polarity_t polarityIn)
{
	cxa_assert(gpioIn);
	cxa_assert( (portIn == CXA_BLE112_GPIO_PORT_0) ||
				(portIn == CXA_BLE112_GPIO_PORT_1) ||
				(portIn == CXA_BLE112_GPIO_PORT_2) );
	cxa_assert(pinNumIn < 8);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );

	// save our references
	gpioIn->port = portIn;
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = polarityIn;
	gpioIn->hasBeenSeld = false;

	// initialize our super class
	cxa_gpio_init(&gpioIn->super, scm_setDirection, scm_getDirection, scm_setPolarity, scm_getPolarity, scm_setValue, scm_getValue);

	// make sure we're set for GPIO
	setSelToGpio(gpioIn);

	// set our initial direction
	cxa_gpio_setDirection(&gpioIn->super, CXA_GPIO_DIR_INPUT);
}


void cxa_ble112_gpio_init_output(cxa_ble112_gpio_t *const gpioIn, cxa_ble112_gpio_port_t portIn, const uint8_t pinNumIn, const cxa_gpio_polarity_t polarityIn, const bool initValIn)
{
	cxa_assert(gpioIn);
	cxa_assert( (portIn == CXA_BLE112_GPIO_PORT_0) ||
					(portIn == CXA_BLE112_GPIO_PORT_1) ||
					(portIn == CXA_BLE112_GPIO_PORT_2) );
	cxa_assert(pinNumIn < 8);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );

	// save our references
	gpioIn->port = portIn;
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = polarityIn;
	gpioIn->hasBeenSeld = false;

	// initialize our super class
	cxa_gpio_init(&gpioIn->super, scm_setDirection, scm_getDirection, scm_setPolarity, scm_getPolarity, scm_setValue, scm_getValue);

	// make sure we're set for GPIO
	setSelToGpio(gpioIn);

	// set our initial value (before we set direction to avoid glitches)
	cxa_gpio_setValue(&gpioIn->super, initValIn);

	// now set our direction
	cxa_gpio_setDirection(&gpioIn->super, CXA_GPIO_DIR_OUTPUT);
}


void cxa_ble112_gpio_init_safe(cxa_ble112_gpio_t *const gpioIn, cxa_ble112_gpio_port_t portIn, const uint8_t pinNumIn)
{
	cxa_assert(gpioIn);
	cxa_assert( (portIn == CXA_BLE112_GPIO_PORT_0) ||
					(portIn == CXA_BLE112_GPIO_PORT_1) ||
					(portIn == CXA_BLE112_GPIO_PORT_2) );
	cxa_assert(pinNumIn < 8);

	// save our references
	gpioIn->port = portIn;
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = CXA_GPIO_POLARITY_NONINVERTED;
	gpioIn->hasBeenSeld = false;

	// initialize our super class
	cxa_gpio_init(&gpioIn->super, scm_setDirection, scm_getDirection, scm_setPolarity, scm_getPolarity, scm_setValue, scm_getValue);

	// don't set any value or direction...just leave everything as it is
}


// ******** local function implementations ********
static void setSelToGpio(cxa_ble112_gpio_t *const gpioIn)
{
	switch( gpioIn->port )
	{
		case CXA_BLE112_GPIO_PORT_0:
			APCFG &= ~(1 << gpioIn->pinNum);		// analog peripheral config
			P0SEL &= ~(1 << gpioIn->pinNum);
			break;

		case CXA_BLE112_GPIO_PORT_1:
			P1SEL &= ~(1 << gpioIn->pinNum);
			break;

		case CXA_BLE112_GPIO_PORT_2:
			P2SEL &= ~(1 << gpioIn->pinNum);
			break;
	}

	gpioIn->hasBeenSeld = true;
}


static void scm_setDirection(cxa_gpio_t *const superIn, const cxa_gpio_direction_t dirIn)
{
	cxa_assert(superIn);
	cxa_assert( (dirIn == CXA_GPIO_DIR_INPUT) ||
				(dirIn == CXA_GPIO_DIR_OUTPUT) );

	// get a pointer to our class
	cxa_ble112_gpio_t *const gpioIn = (cxa_ble112_gpio_t*)superIn;

	// make sure we're set for GPIO
	setSelToGpio(gpioIn);

	switch( gpioIn->port )
	{
		case CXA_BLE112_GPIO_PORT_0:
			if( dirIn == CXA_GPIO_DIR_OUTPUT ) P0DIR |= (1 << gpioIn->pinNum);
			else P0DIR &= ~(1 << gpioIn->pinNum);
			break;

		case CXA_BLE112_GPIO_PORT_1:
			if( dirIn == CXA_GPIO_DIR_OUTPUT ) P1DIR |= (1 << gpioIn->pinNum);
			else P1DIR &= ~(1 << gpioIn->pinNum);
			break;

		case CXA_BLE112_GPIO_PORT_2:
			if( dirIn == CXA_GPIO_DIR_OUTPUT ) P2DIR |= (1 << gpioIn->pinNum);
			else P2DIR &= ~(1 << gpioIn->pinNum);
			break;
	}
}


static cxa_gpio_direction_t scm_getDirection(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_ble112_gpio_t *const gpioIn = (cxa_ble112_gpio_t*)superIn;

	cxa_gpio_direction_t retVal = CXA_GPIO_DIR_UNKNOWN;
	switch( gpioIn->port )
	{
		case CXA_BLE112_GPIO_PORT_0:
			retVal = ((P0DIR & (1 << gpioIn->pinNum)) ? CXA_GPIO_DIR_OUTPUT : CXA_GPIO_DIR_INPUT);
			break;

		case CXA_BLE112_GPIO_PORT_1:
			retVal = ((P1DIR & (1 << gpioIn->pinNum)) ? CXA_GPIO_DIR_OUTPUT : CXA_GPIO_DIR_INPUT);
			break;

		case CXA_BLE112_GPIO_PORT_2:
			retVal = ((P2DIR & (1 << gpioIn->pinNum)) ? CXA_GPIO_DIR_OUTPUT : CXA_GPIO_DIR_INPUT);
			break;
	}

	return retVal;
}


static void scm_setPolarity(cxa_gpio_t *const superIn, const cxa_gpio_polarity_t polarityIn)
{
	cxa_assert(superIn);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );

	// get a pointer to our class
	cxa_ble112_gpio_t *const gpioIn = (cxa_ble112_gpio_t*)superIn;

	gpioIn->polarity = polarityIn;
}


static cxa_gpio_polarity_t scm_getPolarity(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_ble112_gpio_t *const gpioIn = (cxa_ble112_gpio_t*)superIn;

	return gpioIn->polarity;
}


static void scm_setValue(cxa_gpio_t *const superIn, const bool valIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_ble112_gpio_t *const gpioIn = (cxa_ble112_gpio_t*)superIn;

	switch( gpioIn->port )
	{
		case CXA_BLE112_GPIO_PORT_0:
			P0 = (P0 & ~(1 << gpioIn->pinNum)) |
						(((gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !valIn : valIn) << gpioIn->pinNum);
			break;

		case CXA_BLE112_GPIO_PORT_1:
			P1 = (P1 & ~(1 << gpioIn->pinNum)) |
						(((gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !valIn : valIn) << gpioIn->pinNum);
			break;

		case CXA_BLE112_GPIO_PORT_2:
			P2 = (P2 & ~(1 << gpioIn->pinNum)) |
						(((gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !valIn : valIn) << gpioIn->pinNum);
			break;
	}
}


static bool scm_getValue(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_ble112_gpio_t *const gpioIn = (cxa_ble112_gpio_t*)superIn;

	bool retVal = false;
	switch( gpioIn->port )
	{
		case CXA_BLE112_GPIO_PORT_0:
			retVal = (cxa_gpio_getDirection(superIn) == CXA_GPIO_DIR_OUTPUT) ?
					((P0 & (1 << gpioIn->pinNum)) >> gpioIn->pinNum) :
					((P0 & (1 << gpioIn->pinNum)) >> gpioIn->pinNum);
			break;

		case CXA_BLE112_GPIO_PORT_1:
			retVal = (cxa_gpio_getDirection(superIn) == CXA_GPIO_DIR_OUTPUT) ?
					((P1 & (1 << gpioIn->pinNum)) >> gpioIn->pinNum) :
					((P1 & (1 << gpioIn->pinNum)) >> gpioIn->pinNum);
			break;

		case CXA_BLE112_GPIO_PORT_2:
			retVal = (cxa_gpio_getDirection(superIn) == CXA_GPIO_DIR_OUTPUT) ?
					((P2 & (1 << gpioIn->pinNum)) >> gpioIn->pinNum) :
					((P2 & (1 << gpioIn->pinNum)) >> gpioIn->pinNum);
			break;
	}

	return (gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !retVal : retVal;
}
