/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */


// ******** includes ********
#include <cxa_assert.h>
#include <em_rtcc.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
uint32_t cxa_timeBase_getCount_us(void)
{
	return (RTCC_CounterGet() / 32768.0) * 1E6;
}


uint32_t cxa_timeBase_getMaxCount_us(void)
{
	return 131072;
}


// ******** local function implementations ********
