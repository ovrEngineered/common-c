/**
 * @file
 * This file contains an implementation of a time differential. Time differentials
 * allow the user to judge the passage of time (as compared to a reference time base).
 *
 * @note This object should work across all architecture-specific implementations
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * // initialize the time base here
 *
 * ...
 *
 * // initialize the timeDiff
 * cxa_timeDiff_t td_blink;
 * cxa_timeDiff_init(&td_blink, true);
 *
 * ...
 *
 * while(true)
 * {
 *    if( cxa_timeDiff_isElapsed_recurring_ms(&td_blink, 100) )
 *    {
 *       // blink an LED
 *    }
 * }
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
#ifndef CXA_TIMEDIFF_H_
#define CXA_TIMEDIFF_H_


// ******** includes ********
#include <stdint.h>
#include <stdbool.h>
#include <cxa_timeBase.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct 
{
	uint32_t startTime_us;
}cxa_timeDiff_t;


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes the timeDiff using the reference timeBase
 *
 * @param[in] tdIn the pre-allocated timeDiff object
 */
void cxa_timeDiff_init(cxa_timeDiff_t *const tdIn);

/**
 * @public
 * @brief Sets the "startTime" of the timeDiff to the current value of the
 * reference timeBase
 *
 * Once a new startTime has been set, the varying isElapsed functions will
 * return false until the timeBase indicates that enough time has elapsed.
 *
 * @param[in] tdIn the pre-initialized timeDiff
 */
void cxa_timeDiff_setStartTime_now(cxa_timeDiff_t *const tdIn);

/**
 * @public
 *
 * @param[in] tdIn the pre-initialized timeDiff
 *
 * @return the amount of time (in milliseconds) since a call to
 * 		setStartTime_now (as indicated by the reference timeBase)
 */
uint32_t cxa_timeDiff_getElapsedTime_ms(cxa_timeDiff_t *const tdIn);

/**
 * @public
 *
 * @param[in] tdIn the pre-initialized timeDiff
 * @param[in] msIn the desired number of milliseconds
 *
 * @return true if the specified amount of time has elapsed since the last
 * 		call to setStartTime_now. Once true is returned, this timeDiff
 * 		will return true until the following occurs:
 * 		1. setStartTime_now is called again
 * 		2. the timeBase "rolls" over
 */
bool cxa_timeDiff_isElapsed_ms(cxa_timeDiff_t *const tdIn, uint32_t msIn);

/**
 * @public
 * This is a convenience method which combines calls to isElapsed_ms and
 * setStartTime_now.
 *
 * @param[in] tdIn the pre-initialized timeDiff
 * @param[in] msIn the desired number of milliseconds
 *
 * @return true if the specified amount of time has elapsed since the last
 * 		call to setStartTime_now. Once true is returned, this function will
 * 		automatically call setStartTime_now. Thus, the next call to any
 * 		isElapsed function will likely return false (unless it is a small
 * 		time difference).
 */
bool cxa_timeDiff_isElapsed_recurring_ms(cxa_timeDiff_t *const tdIn, uint32_t msIn);


#endif // CXA_TIMEBASE_H_
