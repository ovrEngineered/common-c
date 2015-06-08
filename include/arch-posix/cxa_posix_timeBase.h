/**
 * @file
 * This file contains prototypes and an architecture-specific implementation of a 
 * timeBase object for the X86 processors. A timeBase object is essentially a
 * monotonically increasing counter which can be used to calculate macro-scale,
 * relative timeouts (on the order of seconds and minutes).
 *
 * @note This file contains functionality in addition to that already provided in @ref cxa_timeBase.h
 *		
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * cxa_x86posix_timeBase_t myTimeBase;
 * cxa_x86posix_timeBase_init(&myTimeBase);
 *
 * ...
 *
 * // now use the cxa_timeBase.h common functionality to get the current time
 * uint32_t currTime_us = cxa_timeBase_getCount_us(&myTimeBase.super);
 * @endcode
 *
 *
 * @copyright 2013-2015 opencxa.org
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
#ifndef CXA_POSIX_TIMEBASE_H_
#define CXA_POSIX_TIMEBASE_H_


// ******** includes ********
#include <stdint.h>
#include <cxa_timeBase.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @private
 */
struct cxa_timeBase
{
	void* _placeHolder;
};


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes a timeBase object to use the system's monotonic clock
 * (CLOCK_MONOTONIC) for timing with microSecond resolution.
 *
 * @param[in] tbIn pointer to a pre-allocated POSIX timeBase object
 */
void cxa_posix_timeBase_init(cxa_timeBase_t *const tbIn);


#endif // CXA_POSIX_TIMEBASE_H_
