/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_ATMEGA_TIMEBASE_H_
#define CXA_ATMEGA_TIMEBASE_H_


// ******** includes ********
#include <cxa_atmega_timer.h>
#include <cxa_timeBase.h>


// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
/**
 * @public
 */
void cxa_atmega_timeBase_initWithTimer8(cxa_atmega_timer_t *const timerIn);


#endif
