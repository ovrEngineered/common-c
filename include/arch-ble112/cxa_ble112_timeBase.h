/**
 * @file
 * This file contains prototypes and an architecture-specific implementation of a
 * timeBase object for the BLE112 C SDK. A timeBase object is essentially a
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
#ifndef CXA_BLE112_TIMEBASE_H_
#define CXA_BLE112_TIMEBASE_H_


// ******** includes ********
#include <stdint.h>
#include <cxa_timeBase.h>


// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes a timeBase object to use the system's
 * 24-bit sleep timer
 */
void cxa_ble112_timeBase_init(void);


#endif // CXA_BLE112_TIMEBASE_H_
