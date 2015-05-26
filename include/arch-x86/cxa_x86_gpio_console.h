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
#ifndef CXA_X86_GPIO_H_
#define CXA_X86_GPIO_H_


/**
 * @file <description>
 * @author Christopher Armenio
 */


// ******** includes ********
#include <cxa_gpio.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct
{
	cxa_gpio_t super;

	const char *name;
	cxa_gpio_direction_t currDir;
	bool currVal;
}cxa_x86_gpio_t;


// ******** global function prototypes ********
void cxa_x86_gpio_init_input(cxa_x86_gpio_t *const gpioIn, const char *nameIn);
void cxa_x86_gpio_init_output(cxa_x86_gpio_t *const gpioIn, const char *nameIn, const bool initValIn);
void cxa_x86_gpio_init_safe(cxa_x86_gpio_t *const gpioIn, const char *nameIn);


#endif // CXA_X86_GPIO_H_
