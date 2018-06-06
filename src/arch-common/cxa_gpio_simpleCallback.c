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
#include "cxa_gpio_simpleCallback.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
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
void cxa_gpio_simpleCallback_init_input(cxa_gpio_simpleCallback_t *const gpioIn, const int userIdIn, 
                                        cxa_gpio_simpleCallback_getValue_t cb_getValueIn,
                                        cxa_gpio_simpleCallback_setValue_t cb_setValueIn,
                                        const cxa_gpio_polarity_t polarityIn)
{
	cxa_assert(gpioIn);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );

	// save our references
	gpioIn->userId = userIdIn;
	gpioIn->polarity = polarityIn;
    gpioIn->cb_getValue = cb_getValueIn;
    gpioIn->cb_setValue = cb_setValueIn;
    
    gpioIn->dir = CXA_GPIO_DIR_INPUT;
    
    cxa_gpio_init(&gpioIn->super, scm_setDirection, scm_getDirection, scm_setPolarity, scm_getPolarity, scm_setValue, scm_getValue, NULL);
}


// ******** local function implementations ********
static void scm_setDirection(cxa_gpio_t *const superIn, const cxa_gpio_direction_t dirIn)
{
	cxa_gpio_simpleCallback_t *const gpioIn = (cxa_gpio_simpleCallback_t *const)superIn;
    cxa_assert(gpioIn);
    
	cxa_assert( (dirIn == CXA_GPIO_DIR_INPUT) ||
				(dirIn == CXA_GPIO_DIR_OUTPUT) );
	
	cxa_assert_msg(false, "unimplemented");
}


static cxa_gpio_direction_t scm_getDirection(cxa_gpio_t *const superIn)
{
    cxa_gpio_simpleCallback_t *const gpioIn = (cxa_gpio_simpleCallback_t *const)superIn;
    cxa_assert(gpioIn);
	
	return gpioIn->dir;
}


static void scm_setPolarity(cxa_gpio_t *const superIn, const cxa_gpio_polarity_t polarityIn)
{
	cxa_gpio_simpleCallback_t *const gpioIn = (cxa_gpio_simpleCallback_t *const)superIn;
    cxa_assert(gpioIn);
    
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );
    
    gpioIn->polarity = polarityIn;
}


static cxa_gpio_polarity_t scm_getPolarity(cxa_gpio_t *const superIn)
{
    cxa_gpio_simpleCallback_t *const gpioIn = (cxa_gpio_simpleCallback_t *const)superIn;
    cxa_assert(gpioIn);
	
	return gpioIn->polarity;
}


static void scm_setValue(cxa_gpio_t *const superIn, const bool valIn)
{
	cxa_gpio_simpleCallback_t *const gpioIn = (cxa_gpio_simpleCallback_t *const)superIn;
    cxa_assert(gpioIn);
    cxa_assert(gpioIn->cb_getValue);

    gpioIn->cb_setValue(&gpioIn->super, gpioIn->userId, (gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !valIn : valIn);
	gpioIn->lastVal = valIn;
}


static bool scm_getValue(cxa_gpio_t *const superIn)
{
	cxa_gpio_simpleCallback_t *const gpioIn = (cxa_gpio_simpleCallback_t *const)superIn;
    cxa_assert(gpioIn);
    cxa_assert(gpioIn->cb_getValue);
    
    bool retVal = (gpioIn->dir == CXA_GPIO_DIR_INPUT) ? gpioIn->cb_getValue(&gpioIn->super, gpioIn->userId) : gpioIn->lastVal;
    return (gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !retVal : retVal;
}