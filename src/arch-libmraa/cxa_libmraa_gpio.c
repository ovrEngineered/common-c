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
 */
#include "cxa_libmraa_gpio.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdio.h>
#include <cxa_assert.h>
#include <cxa_config.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_libmraa_gpio_init_input(cxa_libmraa_gpio_t *const gpioIn, int mraaPinIn)
{
	cxa_assert(gpioIn);

	// setup our gpio context
	gpioIn->gpio = mraa_gpio_init(mraaPinIn);

	// set our initial direction
	cxa_gpio_setDirection(&gpioIn->super, CXA_GPIO_DIR_INPUT);
}


void cxa_libmraa_gpio_init_output(cxa_libmraa_gpio_t *const gpioIn, int mraaPinIn, const bool initValIn)
{
	cxa_assert(gpioIn);

	// setup our gpio context
	gpioIn->gpio = mraa_gpio_init(mraaPinIn);

	// set our initial value
	cxa_gpio_setValue(&gpioIn->super, initValIn);
	// now set our initial direction
	cxa_gpio_setDirection(&gpioIn->super, CXA_GPIO_DIR_OUTPUT);
}


void cxa_libmraa_gpio_init_safe(cxa_libmraa_gpio_t *const gpioIn, int mraaPinIn)
{
	cxa_assert(gpioIn);

	// setup our gpio context
	gpioIn->gpio = mraa_gpio_init(mraaPinIn);
	gpioIn->currDir = CXA_GPIO_DIR_UNKNOWN;
}


void cxa_gpio_setDirection(cxa_gpio_t *const superIn, const cxa_gpio_direction_t dirIn)
{
	cxa_assert(superIn);
	cxa_assert( (dirIn == CXA_GPIO_DIR_INPUT) ||
				(dirIn == CXA_GPIO_DIR_OUTPUT) );

	// get a pointer to our class
	cxa_libmraa_gpio_t *const gpioIn = (cxa_libmraa_gpio_t *const)superIn;
	gpioIn->currDir = dirIn;

	switch(gpioIn->currDir)
	{
		case CXA_GPIO_DIR_INPUT:
			mraa_gpio_dir(gpioIn->gpio, MRAA_GPIO_IN);
			break;

		case CXA_GPIO_DIR_OUTPUT:
			mraa_gpio_dir(gpioIn->gpio, MRAA_GPIO_OUT);
			break;

		default: break;
	}
}


cxa_gpio_direction_t cxa_gpio_getDirection(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_libmraa_gpio_t *const gpioIn = (cxa_libmraa_gpio_t *const)superIn;

	return gpioIn->currDir;
}


void cxa_gpio_setValue(cxa_gpio_t *const superIn, const bool valIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_libmraa_gpio_t *const gpioIn = (cxa_libmraa_gpio_t *const)superIn;
	gpioIn->currVal = valIn;
	mraa_gpio_write(gpioIn->gpio, valIn);
}


bool cxa_gpio_getValue(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_libmraa_gpio_t *const gpioIn = (cxa_libmraa_gpio_t *const)superIn;

	return mraa_gpio_read(gpioIn->gpio) ? true : false;
}


void cxa_gpio_toggle(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_libmraa_gpio_t *const gpioIn = (cxa_libmraa_gpio_t *const)superIn;

	cxa_gpio_setValue(superIn, !gpioIn->currVal);
}


// ******** local function implementations ********

