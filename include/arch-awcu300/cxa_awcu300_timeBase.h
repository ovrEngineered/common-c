/**
 * @file
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
#ifndef CXA_AWCU300_TIMEBASE_H_
#define CXA_AWCU300_TIMEBASE_H_


// ******** includes ********
#include <cxa_timeBase.h>
#include <cxa_awcu300_timer32.h>


// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
void cxa_awcu300_timeBase_init_timer32(cxa_awcu300_timer32_t *const timerIn);


#endif // CXA_AWCU300_TIMEBASE_H_
