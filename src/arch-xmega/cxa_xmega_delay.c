/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_delay.h"


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
