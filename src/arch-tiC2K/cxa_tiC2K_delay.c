/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_delay.h"

// ******** includes ********


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_delay_ms(uint16_t delay_msIn)
{
    /* DEVICEDELAY macro is in microseconds, so multiply
     * requested microseconds by 1000 for microseconds.  */
	DEVICE_DELAY_US(((uint32_t)delay_msIn) * 1000);
}


// ******** local function implementations ********
