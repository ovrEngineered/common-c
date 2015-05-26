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
const char* cxa_gpio_direction_toString(const cxa_gpio_direction_t dirIn)
{
	cxa_assert( (dirIn == CXA_GPIO_DIR_INPUT) ||
				(dirIn == CXA_GPIO_DIR_OUTPUT) );

	const char* retVal = NULL;
	switch( dirIn )
	{
		case CXA_GPIO_DIR_INPUT:
			retVal = "input";
			break;

		case CXA_GPIO_DIR_OUTPUT:
			retVal = "output";
			break;
		
		default:
			retVal = "unknown";
			break;
	}

	return retVal;
}


// ******** local function implementations ********

