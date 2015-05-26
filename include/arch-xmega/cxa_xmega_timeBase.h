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
 *
 *
 * @copyright 2013-2014 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_XMEGA_TIMEBASE_H_
#define CXA_XMEGA_TIMEBASE_H_


// ******** includes ********
#include <stdint.h>
#include <cxa_timeBase.h>
#include <cxa_xmega_timer32.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_xmega_timeBase_t object
 */
typedef struct cxa_xmega_timeBase cxa_xmega_timeBase_t;


/**
 * @private
 */
struct cxa_xmega_timeBase
{
	cxa_timeBase_t super;
	cxa_xmega_timer32_t *timer;
};


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes a timeBase object to use a pre-configured ::cxa_xmega_timer32_t object as a counter.
 * The ::cxa_xmega_timer32_t should have already been configured as a free-running timer with a
 * sane resolution (eg. such a resolution that the overflow period of the 32-bit timer is signficantly
 * longer than the longest time period measured with this timeBase)
 *
 * @param[in] tbIn pointer to a pre-allocated XMega timeBase object
 * @param[in] timerIn pre-configured XMega 32-bit timer
 */
void cxa_xmega_timeBase_init_timer32(cxa_xmega_timeBase_t *const tbIn, cxa_xmega_timer32_t *const timerIn);


#endif // CXA_XMEGA_TIMEBASE_H_
