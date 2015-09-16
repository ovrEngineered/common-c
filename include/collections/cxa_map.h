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
#ifndef CXA_MAP_H_
#define CXA_MAP_H_


// ******** includes ********
#include <stdlib.h>
#include <cxa_array.h>


// ******** global macro definitions ********
#define cxa_map_decl(mapNameIn, maxNumEntriesIn, keyDataTypeIn, valueDataTypeIn)	 										\
	uint8_t cxa_map_##mapNameIn##_raw[sizeof(cxa_map_t)+(maxNumEntriesIn*(sizeof(keyDataTypeIn)+sizeof(valueDataTypeIn)))];	\
	cxa_map_t* mapNameIn = (cxa_map_t*)cxa_map_##mapNameIn##_raw;


#define cxa_map_initStd(mapIn, maxNumEntriesIn, keyDataTypeIn, valueDataTypeIn)				cxa_map_init((mapIn), (maxNumEntriesIn), sizeof(keyDataTypeIn), sizeof(valueDataTypeIn))


// ******** global type definitions *********
typedef struct
{
	size_t maxNumEntries;
	size_t keyDataTypeSize_bytes;
	size_t valueDataTypeSize_bytes;

	cxa_array_t entries;
	uint8_t entries_raw[];
}cxa_map_t;


// ******** global function prototypes ********
void cxa_map_init(cxa_map_t *const mapIn, size_t maxNumEntriesIn, size_t keyDataTypeSize_bytesIn, size_t valueDataTypeSize_bytesIn);


#endif /* CXA_MAP_H_ */
