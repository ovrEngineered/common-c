/**
 * @file
 *
 * @note This object should work across all architecture-specific implementations
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * @endcode
 *
 *
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
#include "cxa_map.h"


// ******** includes ********
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_map_init(cxa_map_t *const mapIn, size_t maxNumEntriesIn, size_t keyDataTypeSize_bytesIn, size_t valueDataTypeSize_bytesIn)
{
	cxa_assert(mapIn);

	// save our references
	mapIn->maxNumEntries = maxNumEntriesIn;
	mapIn->keyDataTypeSize_bytes = keyDataTypeSize_bytesIn;
	mapIn->valueDataTypeSize_bytes = valueDataTypeSize_bytesIn;

	// setup our array of entries
	cxa_array_init(&mapIn->entries, (mapIn->keyDataTypeSize_bytes+mapIn->valueDataTypeSize_bytes), mapIn->entries_raw, (maxNumEntriesIn * (mapIn->keyDataTypeSize_bytes + mapIn->valueDataTypeSize_bytes)));
}


// ******** local function implementations ********
