/**
 * @file
 * @copyright 2015 opencxa.org
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
#ifndef CXA_ESP8266_TIMEBASE_H_
#define CXA_ESP8266_TIMEBASE_H_


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
 */
void cxa_esp8266_timeBase_init(cxa_timeBase_t *const tbIn);


#endif // CXA_ESP826_TIMEBASE_H_
