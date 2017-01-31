/**
 * @file
 * @copyright 2017 opencxa.org
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
#ifndef CXA_DUMMY_RGBLED_H_
#define CXA_DUMMY_RGBLED_H_


// ******** includes ********
#include <cxa_rgbLed.h>
#include <cxa_logger_header.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_dummy_rgbLed cxa_dummy_rgbLed_t;


/**
 * @private
 */
struct cxa_dummy_rgbLed
{
	cxa_rgbLed_t super;

	cxa_logger_t logger;
};


// ******** global function prototypes ********
void cxa_dummy_rgbLed_init(cxa_dummy_rgbLed_t *const ledIn, const char *const nameIn);


#endif
