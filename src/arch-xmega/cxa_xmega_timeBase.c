/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_xmega_timeBase.h"


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
