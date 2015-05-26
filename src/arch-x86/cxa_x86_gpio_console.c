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
#include "cxa_x86_gpio.h"


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
void cxa_x86_gpio_init_input(cxa_x86_gpio_t *const gpioIn, const char *nameIn)
{
	cxa_assert(gpioIn);
	cxa_assert(nameIn);

	// save our references
	gpioIn->name = nameIn;

	// set our initial direction
	cxa_gpio_setDirection(&gpioIn->super, CXA_GPIO_DIR_INPUT);
}


void cxa_x86_gpio_init_output(cxa_x86_gpio_t *const gpioIn, const char *nameIn, const bool initValIn)
{
	cxa_assert(gpioIn);
	cxa_assert(nameIn);

	// save our references
	gpioIn->name = nameIn;

	// set our initial value
	cxa_gpio_setValue(&gpioIn->super, initValIn);
	// now set our initial direction
	cxa_gpio_setDirection(&gpioIn->super, CXA_GPIO_DIR_OUTPUT);
}


void cxa_x86_gpio_init_safe(cxa_x86_gpio_t *const gpioIn, const char *nameIn)
{
	cxa_assert(gpioIn);
	cxa_assert(nameIn);

	// save our references
	gpioIn->name = nameIn;

	// we don't know our current direction
	gpioIn->currDir = CXA_GPIO_DIR_UNKNOWN;
}


void cxa_gpio_setDirection(cxa_gpio_t *const superIn, const cxa_gpio_direction_t dirIn)
{
	cxa_assert(superIn);
	cxa_assert( (dirIn == CXA_GPIO_DIR_INPUT) ||
				(dirIn == CXA_GPIO_DIR_OUTPUT) );

	// get a pointer to our class
	cxa_x86_gpio_t *const gpioIn = (cxa_x86_gpio_t *const)superIn;

	gpioIn->currDir = dirIn;
	printf("gpio[%p] setDir: %s\r\n", gpioIn, cxa_gpio_direction_toString(gpioIn->currDir));
}


cxa_gpio_direction_t cxa_gpio_getDirection(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_x86_gpio_t *const gpioIn = (cxa_x86_gpio_t *const)superIn;

	return gpioIn->currDir;
}


void cxa_gpio_setValue(cxa_gpio_t *const superIn, const bool valIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_x86_gpio_t *const gpioIn = (cxa_x86_gpio_t *const)superIn;

	gpioIn->currVal = valIn;
	printf("gpio[%p] setVal: %d\r\n", gpioIn, gpioIn->currVal);
}


bool cxa_gpio_getValue(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_x86_gpio_t *const gpioIn = (cxa_x86_gpio_t *const)superIn;

	return gpioIn->currVal;
}


void cxa_gpio_toggle(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_x86_gpio_t *const gpioIn = (cxa_x86_gpio_t *const)superIn;

	gpioIn->currVal = !gpioIn->currVal;
}


// ******** local function implementations ********

