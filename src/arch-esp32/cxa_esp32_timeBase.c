/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include <cxa_esp32_timeBase.h>


// ******** includes ********
#include <esp_timer.h>

#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_esp32_timeBase_init(void)
{
}


uint32_t cxa_timeBase_getCount_us(void)
{
	return esp_timer_get_time() % UINT32_MAX;
}


uint32_t cxa_timeBase_getMaxCount_us(void)
{
	return UINT32_MAX;
}


// ******** local function implementations ********
