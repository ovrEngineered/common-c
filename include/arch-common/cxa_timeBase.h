/**
 * @file
 * This file contains prototypes and a top-level implementation of a timeBase object.
 * A timeBase object is essentially a monotonically increasing counter which can be
 * used to calculate macro-scale, relative timeouts (on the order of seconds and minutes).
 *
 * In most implementations, the timeBase is backed by a monotonically increasing counter
 * of limited range (16 or 32 bit). In these cases, the counter will simply start counting
 * when initialized and continue to count up, until it overflows at ::cxa_timeBase_getMaxCount_us.
 * Due to this potential overflow, care must be taken to calculate timeouts while taking the
 * maximum value of the counter into account. It is not possible to judge elapsed time
 * beyond the range returned by ::cxa_timeBase_getMaxCount_us.
 *
 * @note This file contains the base functionality for a timeBase object available across all architectures. Additional
 *		functionality, including initialization is available in the architecture-specific implementation.
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * cxa_<arch>_timeBase_t myTimeBase;
 * // initialization for architecture specific implementation
*
 * ...
 *
 * // get the current time in microseconds
 * uint32_t currTime_us = cxa_timeBase_getCount_us(&myTimeBase);
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
#ifndef CXA_TIMEBASE_H_
#define CXA_TIMEBASE_H_


// ******** includes ********
#include <stdint.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_timeBase_t object
 */
typedef struct cxa_timeBase cxa_timeBase_t;


/**
 * @private
 */
struct cxa_timeBase
{
};


// ******** global function prototypes ********
/**
 * @public
 * @brief Returns the current monotonic, relative time in microseconds.
 *
 * @param[in] superIn pointer to a pre-initialized timeBase object 
 *
 * @return the current time of the timeBase, in microseconds
 */
uint32_t cxa_timeBase_getCount_us(cxa_timeBase_t *const superIn);


/**
 * @public
 * @brief Returns the maximum value of the timeBase, in microseconds.
 * After reaching this value, the underlying timing mechanism will overflow to 0.
 * It is not possible to measure time periods longer than this value.
 *
 * @param[in] superIn pointer to a pre-initialized timeBase object 
 *
 * @return the maximum time period measureable by this timeBase
 */
uint32_t cxa_timeBase_getMaxCount_us(cxa_timeBase_t *const superIn);


#endif // CXA_TIMEBASE_H_
