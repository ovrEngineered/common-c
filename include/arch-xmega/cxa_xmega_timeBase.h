/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
 
/**
 * @file
 * This file contains prototypes and an architecture-specific implementation of a
 * timeBase object for the XMega processor. A timeBase object is essentially a
 * monotonically increasing counter which can be used to calculate macro-scale,
 * relative timeouts (on the order of seconds and minutes).
 *
 * @note This file contains functionality restricted to the CXA Atmel XMega implementation.
 *
 * @note This file contains functionality in addition to that already provided in @ref cxa_timeBase.h
 *
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * cxa_xmega_timeBase_t myTimeBase;
 * // timer: pre-initialized cxa_xmega_timer32_t object
 * cxa_xmega_timeBase_init_timer32(&myTimeBase, &timer);
 *
 * ...
 *
 * // now use the cxa_timeBase.h common functionality to get the current time
 * uint32_t currTime_us = cxa_timeBase_getCount_us(&myTimeBase.super);
 * @endcode
 */
#ifndef CXA_XMEGA_TIMEBASE_H_
#define CXA_XMEGA_TIMEBASE_H_


// ******** includes ********
#include <stdint.h>
#include <cxa_xmega_timer32.h>


// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes the timeBase object to use a pre-configured ::cxa_xmega_timer32_t object as a counter.
 * The ::cxa_xmega_timer32_t should have already been configured as a free-running timer with a
 * sane resolution (eg. such a resolution that the overflow period of the 32-bit timer is signficantly
 * longer than the longest time period measured with the timeBase)
 *
 * @param[in] timerIn pre-configured XMega 32-bit timer
 */
void cxa_xmega_timeBase_init_timer32(cxa_xmega_timer32_t *const timerIn);


#endif // CXA_XMEGA_TIMEBASE_H_
