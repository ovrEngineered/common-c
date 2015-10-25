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
#include "cxa_esp8266_timeBase.h"


// ******** includes ********
#include <stddef.h>
#include <user_interface.h>
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_esp8266_timeBase_init(cxa_timeBase_t *const tbIn)
{
	cxa_assert(tbIn);
	
	// nothing to do here
}


uint32_t cxa_timeBase_getCount_us(cxa_timeBase_t *const superIn)
{
	cxa_assert(superIn);

	return system_get_time();
}


uint32_t cxa_timeBase_getMaxCount_us(cxa_timeBase_t *const superIn)
{
	cxa_assert(superIn);
	
	return UINT32_MAX;
}


// ******** local function implementations ********

