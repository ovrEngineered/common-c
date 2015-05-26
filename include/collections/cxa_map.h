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
#ifndef CXA_MAP_H_
#define CXA_MAP_H_


// ******** includes ********
#include <stdlib.h>
#include <stdbool.h>


// ******** global macro definitions ********
#define CXA_MAP_CALC_BUFFER_SIZE(keyTypeIn, valueTypeIn, numElementsIn)					((sizeof(keyTypeIn) + sizeof(valueTypeIn)) * (numElementsIn))


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_map_t object
 */
typedef struct cxa_map cxa_map_t;


/**
 * @private
 */
struct cxa_map
{
	void *bufferLoc;
	size_t insertIndex;

	size_t keySize_bytes;
	size_t valueSize_bytes;
	size_t maxNumElements;
};


// ******** global function prototypes ********
void cxa_map_init(cxa_map_t *const mapIn, size_t keySize_bytesIn, size_t valueSize_bytesIn, void *const bufferLocIn, const size_t bufferMaxSize_bytesIn);

bool cxa_map_put(cxa_map_t *const mapIn, void *keyIn, void *valueIn);

void* cxa_map_put_empty(cxa_map_t *const mapIn, void *keyIn);

bool cxa_map_removeElement(cxa_map_t *const mapIn, void *keyIn);

void* cxa_map_get(cxa_map_t *const mapIn, void *keyIn);

const size_t cxa_map_getSize_elems(cxa_map_t *const mapIn);

const size_t cxa_map_getMaxSize_elems(cxa_map_t *const mapIn);

const bool cxa_map_isFull(cxa_map_t *const mapIn);

const bool cxa_map_isEmpty(cxa_map_t *const mapIn);

void cxa_map_clear(cxa_map_t *const mapIn);

size_t cxa_map_getFreeSize_elems(cxa_map_t *const mapIn);


#endif // CXA_MAP_H_
