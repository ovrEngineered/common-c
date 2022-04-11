/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include <cxa_atmega_timeBase.h>

// ******** includes ********
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void timer8_cb_onOverflow(cxa_atmega_timer_t *const timerIn, void *userVarIn);


// ********  local variable declarations *********
static cxa_atmega_timer_t* timer = NULL;
static uint32_t numOverflows = 0;


// ******** global function implementations ********
void cxa_atmega_timeBase_initWithTimer8(cxa_atmega_timer_t *const timerIn)
{
	cxa_assert(timerIn);

	timer = timerIn;
	cxa_atmega_timer_addListener(timer, timer8_cb_onOverflow, NULL);
	cxa_atmega_timer_enableInterrupt_overflow(timer);
}


uint32_t cxa_timeBase_getCount_us(void)
{
	return (timer != NULL) ? (numOverflows * ((uint32_t)(cxa_atmega_timer_getOverflowPeriod_s(timer) * 1.0E6))) : 0;
}


uint32_t cxa_timeBase_getMaxCount_us(void)
{
	return UINT32_MAX;
}


// ******** local function implementations ********
static void timer8_cb_onOverflow(cxa_atmega_timer_t *const timerIn, void *userVarIn)
{
	numOverflows++;
}
