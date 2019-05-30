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
#include "cxa_gpio.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_gpio_init(cxa_gpio_t *const gpioIn,
				   cxa_gpio_scm_setDirection_t scm_setDirectionIn,
				   cxa_gpio_scm_getDirection_t scm_getDirectionIn,
				   cxa_gpio_scm_setPolarity_t scm_setPolarityIn,
				   cxa_gpio_scm_getPolarity_t scm_getPolarityIn,
				   cxa_gpio_scm_setValue_t scm_setValueIn,
				   cxa_gpio_scm_getValue_t scm_getValueIn,
				   cxa_gpio_scm_enableInterrupt_t scm_enableInterruptIn)
{
	cxa_assert(gpioIn);
	cxa_assert(scm_setDirectionIn);
	cxa_assert(scm_getDirectionIn);
	cxa_assert(scm_setPolarityIn);
	cxa_assert(scm_getPolarityIn);
	cxa_assert(scm_setValueIn);
	cxa_assert(scm_getValueIn);

	// save our references
	gpioIn->scm_setDirection = scm_setDirectionIn;
	gpioIn->scm_getDirection = scm_getDirectionIn;
	gpioIn->scm_setPolarity = scm_setPolarityIn;
	gpioIn->scm_getPolarity = scm_getPolarityIn;
	gpioIn->scm_setValue = scm_setValueIn;
	gpioIn->scm_getValue = scm_getValueIn;
	gpioIn->scm_enableInterrupt = scm_enableInterruptIn;

	// zero-out our callbacks
	gpioIn->cbs.onInterrupt = NULL;
}


void cxa_gpio_setDirection(cxa_gpio_t *const gpioIn, const cxa_gpio_direction_t dirIn)
{
	cxa_assert(gpioIn);
	cxa_assert(gpioIn->scm_setDirection);
	gpioIn->scm_setDirection(gpioIn, dirIn);
}


cxa_gpio_direction_t cxa_gpio_getDirection(cxa_gpio_t *const gpioIn)
{
	cxa_assert(gpioIn);
	cxa_assert(gpioIn->scm_getDirection);
	return gpioIn->scm_getDirection(gpioIn);
}


void cxa_gpio_setPolarity(cxa_gpio_t *const gpioIn, const cxa_gpio_polarity_t polarityIn)
{
	cxa_assert(gpioIn);
	cxa_assert(gpioIn->scm_setPolarity);
	gpioIn->scm_setPolarity(gpioIn, polarityIn);
}


cxa_gpio_polarity_t cxa_gpio_getPolarity(cxa_gpio_t *const gpioIn)
{
	cxa_assert(gpioIn);
	cxa_assert(gpioIn->scm_getPolarity);
	return gpioIn->scm_getPolarity(gpioIn);
}


void cxa_gpio_setValue(cxa_gpio_t *const gpioIn, const bool valIn)
{
	cxa_assert(gpioIn);
	cxa_assert(gpioIn->scm_setValue);
	gpioIn->scm_setValue(gpioIn, valIn);
}


bool cxa_gpio_getValue(cxa_gpio_t *const gpioIn)
{
	cxa_assert(gpioIn);
	cxa_assert(gpioIn->scm_getValue);
	return gpioIn->scm_getValue(gpioIn);
}


void cxa_gpio_toggle(cxa_gpio_t *const gpioIn)
{
	cxa_assert(gpioIn);

	cxa_gpio_setValue(gpioIn, !cxa_gpio_getValue(gpioIn));
}


bool cxa_gpio_enableInterrupt(cxa_gpio_t *const gpioIn, cxa_gpio_interruptType_t intTypeIn, cxa_gpio_cb_onInterrupt_t cbIn, void* userVarIn)
{
	cxa_assert(gpioIn);
	cxa_assert(gpioIn->scm_enableInterrupt);

	// save our callback
	gpioIn->cbs.onInterrupt = cbIn;
	gpioIn->cbs.userVar = userVarIn;

	return gpioIn->scm_enableInterrupt(gpioIn, intTypeIn, cbIn, userVarIn);
}


void cxa_gpio_notify_onInterrupt(cxa_gpio_t *const gpioIn)
{
	cxa_assert(gpioIn);

	if( gpioIn->cbs.onInterrupt != NULL ) gpioIn->cbs.onInterrupt(gpioIn, cxa_gpio_getValue(gpioIn), gpioIn->cbs.userVar);
}


// ******** local function implementations ********

