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
#include "cxa_xmega_timeBase.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********
static cxa_xmega_timer32_t* timer = NULL;


// ******** global function implementations ********
void cxa_xmega_timeBase_init_timer32(cxa_xmega_timer32_t *const timerIn)
{
	cxa_assert(timerIn);
	
	// save our references
	timer = timerIn;
}


uint32_t cxa_timeBase_getCount_us(void)
{
	cxa_assert(timer);
	
	return cxa_xmega_timer32_getCount(timer) * (1000000 / cxa_xmega_timer32_getResolution_cntsPerS(timer));
}


uint32_t cxa_timeBase_getMaxCount_us(void)
{
	cxa_assert(timer);
	
	return cxa_xmega_timer32_getMaxVal_cnts(timer) * (1000000 / cxa_xmega_timer32_getResolution_cntsPerS(timer));
}


// ******** local function implementations ********

