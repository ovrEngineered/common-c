/**
 * @file
 * @copyright 2018 opencxa.org
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
#ifndef CXA_ATMEGA_TIMEBASE_H_
#define CXA_ATMEGA_TIMEBASE_H_


// ******** includes ********
#include <cxa_atmega_timer8.h>
#include <cxa_timeBase.h>


// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
/**
 * @public
 */
void cxa_atmega_timeBase_initWithTimer8(cxa_atmega_timer8_t *const timerIn);


#endif
