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
#include "cxa_delay.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <cxa_xmega_clockController.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
extern void __builtin_avr_delay_cycles(unsigned long);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_delay_ms(uint16_t delay_msIn)
{	
	uint32_t delay_ticks = (cxa_xmega_clockController_getSystemClockFrequency_hz() / 1000) * delay_msIn;
	
	while( delay_ticks >= 10000 )
	{
		__builtin_avr_delay_cycles(10000);
		delay_ticks -= 10000;
	}
	while( delay_ticks >= 1000 )
	{
		__builtin_avr_delay_cycles(1000);
		delay_ticks -= 1000;
	}
	while( delay_ticks >= 100 )
	{
		__builtin_avr_delay_cycles(100);
		delay_ticks -= 100;
	}
	while( delay_ticks != 0 )
	{
		__builtin_avr_delay_cycles(1);
		delay_ticks -= 1;
	}
}


// ******** local function implementations ********

