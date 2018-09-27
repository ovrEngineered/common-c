/**
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
#include "cxa_random.h"


// ******** includes ********
#include <esp_system.h>

#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
uint32_t cxa_random_numberInRange(uint32_t lowerLimitIn, uint32_t upperLimitIn)
{
	cxa_assert(lowerLimitIn < upperLimitIn);

	return lowerLimitIn + (esp_random() / (UINT32_MAX / (upperLimitIn - lowerLimitIn + 1) + 1));
}


// ******** local function implementations ********
