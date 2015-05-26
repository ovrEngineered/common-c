/**
 * @file
 * This file contains functions for delaying continued execution by a fixed amount of time.
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * // start here @ 0sec
 * cxa_delay_ms(1000);
 * // reach here @ 1sec
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
#ifndef CXA_DELAY_H_
#define CXA_DELAY_H_


// ******** includes ********
#include <stdint.h>


// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
/**
 * @public
 * @brief Delays for a fixed number of milliseconds
 *
 * @param[in] delay_msIn the number of milliseconds for which to delay
 */
void cxa_delay_ms(uint16_t delay_msIn);


#endif // CXA_DELAY_H_
