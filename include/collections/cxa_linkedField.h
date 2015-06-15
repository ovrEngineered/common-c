/**
 * @file
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
#ifndef CXA_LINKED_FIELD_H_
#define CXA_LINKED_FIELD_H_


// ******** includes ********
#include <stdbool.h>
#include <cxa_fixedByteBuffer.h>


// ******** global macro definitions ********
#define cxa_linkedField_append_uint8(fbbLfIn, uint8In)						cxa_linkedField_append((fbbLfIn), (uint8_t[]){(uint8In)}, 1)
#define cxa_linkedField_append_uint16LE(fbbLfIn, uint16In)					cxa_linkedField_append((fbbLfIn), (uint8_t[]){((uint8_t)((((uint16_t)(uint16In)) & 0x00FF) >> 0)), ((uint8_t)((((uint16_t)(uint16In)) & 0xFF00) >> 8))}, 2)
#define cxa_linkedField_append_uint32LE(fbbLfIn, uint32In)					cxa_linkedField_append((fbbLfIn), (uint8_t[]){ ((uint8_t)((((uint32_t)(uint32In)) & 0x000000FF) >> 0)), ((uint8_t)((((uint32_t)(uint32In)) & 0x0000FF00) >> 8)), ((uint8_t)((((uint32_t)(uint32In)) & 0x00FF0000) >> 16)), ((uint8_t)((((uint32_t)(uint32In)) & 0xFF000000) >> 24)) }, 4)
#define cxa_linkedField_append_float(fbbLfIn, floatIn)						cxa_linkedField_append((fbbLfIn), (uint8_t*)&(floatIn), 4)
#define cxa_linkedField_append_cString(fbbLfIn, strIn)						cxa_linkedField_append((fbbLfIn), (uint8_t*)(strIn), (strlen(strIn)+1))

#define cxa_linkedField_remove_uint8(fbbLfIn, indexIn)						cxa_linkedField_remove((fbbLfIn), (indexIn), 1)
#define cxa_linkedField_remove_uint16(fbbLfIn, indexIn)						cxa_linkedField_remove((fbbLfIn), (indexIn), 2)
#define cxa_linkedField_remove_uint32(fbbLfIn, indexIn)						cxa_linkedField_remove((fbbLfIn), (indexIn), 4)
#define cxa_linkedField_remove_float(fbbLfIn, indexIn)						cxa_linkedField_remove((fbbLfIn), (indexIn), 4)

#define cxa_linkedField_get_uint8(fbbLfIn, indexIn, uint8Out)				cxa_linkedField_get((fbbLfIn), (indexIn), false, (uint8_t*)&(uint8Out), 1)
#define cxa_linkedField_get_uint16LE(fbbLfIn, indexIn, uint16Out)			cxa_linkedField_get((fbbLfIn), (indexIn), false, (uint8_t*)&(uint16Out), 2)
#define cxa_linkedField_get_uint32LE(fbbLfIn, indexIn, uint32Out)			cxa_linkedField_get((fbbLfIn), (indexIn), false, (uint8_t*)&(uint32Out), 4)
#define cxa_linkedField_get_float(fbbLfIn, indexIn, floatOut)				cxa_linkedField_get((fbbLfIn), (indexIn), false, (uint8_t*)&(floatOut), 4)

#define cxa_linkedField_replace_uint8(fbbLfIn, indexIn, uint8In)			cxa_linkedField_replace((fbbLfIn), (indexIn), (uint8_t[]){(uint8In)}, 1)
#define cxa_linkedField_replace_uint16LE(fbbLfIn, indexIn, uint16In)		cxa_linkedField_replace((fbbLfIn), (indexIn), (uint8_t[]){((uint8_t)((((uint16_t)(uint16In)) & 0x00FF) >> 0)), ((uint8_t)((((uint16_t)(uint16In)) & 0xFF00) >> 8))}, 2)
#define cxa_linkedField_replace_uint32LE(fbbLfIn, indexIn, uint32In)		cxa_linkedField_replace((fbbLfIn), (indexIn), (uint8_t[]){ ((uint8_t)((((uint32_t)(uint32In)) & 0x000000FF) >> 0)), ((uint8_t)((((uint32_t)(uint32In)) & 0x0000FF00) >> 8)), ((uint8_t)((((uint32_t)(uint32In)) & 0x00FF0000) >> 16)), ((uint8_t)((((uint32_t)(uint32In)) & 0xFF000000) >> 24)) }, 4)
#define cxa_linkedField_replace_float(fbbLfIn, indexIn, floatIn)			cxa_linkedField_replace((fbbLfIn), (indexIn), (uint8_t*)&(floatIn), 4)

#define cxa_linkedField_insert_uint8(fbbLfIn, indexIn, uint8In)				cxa_linkedField_insert((fbbLfIn), (indexIn), (uint8_t[]){(uint8In)}, 1)
#define cxa_linkedField_insert_uint16(fbbLfIn, indexIn, uint16In)			cxa_linkedField_insert((fbbLfIn), (indexIn), (uint8_t[]){((uint8_t)((((uint16_t)(uint16In)) & 0x00FF) >> 0)), ((uint8_t)((((uint16_t)(uint16In)) & 0xFF00) >> 8))}, 2)
#define cxa_linkedField_insert_uint32(fbbLfIn, indexIn, uint32In)			cxa_linkedField_insert((fbbLfIn), (indexIn), (uint8_t[]){ ((uint8_t)((((uint32_t)(uint32In)) & 0x000000FF) >> 0)), ((uint8_t)((((uint32_t)(uint32In)) & 0x0000FF00) >> 8)), ((uint8_t)((((uint32_t)(uint32In)) & 0x00FF0000) >> 16)), ((uint8_t)((((uint32_t)(uint32In)) & 0xFF000000) >> 24)) }, 4)
#define cxa_linkedField_insert_float(fbbLfIn, indexIn, floatIn)				cxa_linkedField_insert((fbbLfIn), (indexIn), (uint8_t*)&(floatIn), 4)
#define cxa_linkedField_insert_cString(fbbLfIn, indexIn, strIn)				cxa_linkedField_insert((fbbLfIn), (indexIn), (uint8_t*)(strIn), (strlen(strIn)+1))

#define cxa_linkedField_getStartIndexOfNextField(fbbLfIn)					(cxa_linkedField_getStartIndexInParent(fbbLfIn) + cxa_linkedField_getSize_bytes(fbbLfIn))


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_linkedField_t object
 */
typedef struct cxa_fixedByteBuffer_linkedField cxa_linkedField_t;


/**
 * @private
 */
struct cxa_fixedByteBuffer_linkedField
{
	cxa_fixedByteBuffer_t* parent;

	cxa_linkedField_t* prev;
	cxa_linkedField_t* next;

	size_t startIndex;

	bool isFixedLength;
	size_t maxFixedLength_bytes;

	size_t currSize_bytes;
};


// ******** global function prototypes ********
bool cxa_linkedField_initRoot(cxa_linkedField_t *const fbbLfIn, cxa_fixedByteBuffer_t *const parentFbbIn, const size_t startIndexInParentIn, const size_t initialSize_bytesIn);
bool cxa_linkedField_initRoot_fixedLen(cxa_linkedField_t *const fbbLfIn, cxa_fixedByteBuffer_t *const parentFbbIn, const size_t startIndexInParentIn, const size_t maxLen_bytesIn);

bool cxa_linkedField_initChild(cxa_linkedField_t *const fbbLfIn, cxa_linkedField_t *const prevFbbLfIn, const size_t initialSize_bytesIn);
bool cxa_linkedField_initChild_fixedLen(cxa_linkedField_t *const fbbLfIn, cxa_linkedField_t *const prevFbbLfIn, const size_t maxLen_bytesIn);

bool cxa_linkedField_append(cxa_linkedField_t *const fbbLfIn, uint8_t *const ptrIn, const size_t numBytesIn);
bool cxa_linkedField_remove(cxa_linkedField_t *const fbbLfIn, const size_t indexIn, const size_t numBytesIn);
bool cxa_linkedField_remove_cString(cxa_linkedField_t *const fbbLfIn, const size_t indexIn);

uint8_t* cxa_linkedField_get_pointerToIndex(cxa_linkedField_t *const fbbLfIn, const size_t indexIn);
bool cxa_linkedField_get(cxa_linkedField_t *const fbbLfIn, const size_t indexIn, bool transposeIn, uint8_t *const valOut, const size_t numBytesIn);
bool cxa_linkedField_get_cstring(cxa_linkedField_t *const fbbLfIn, const size_t indexIn, char *const stringOut, size_t maxOutputSize_bytes);

bool cxa_linkedField_replace(cxa_linkedField_t *const fbbLfIn, const size_t indexIn, uint8_t *const ptrIn, const size_t numBytesIn);
bool cxa_linkedField_replace_cstring(cxa_linkedField_t *const fbbLfIn, const size_t indexIn, char *const stringIn);

bool cxa_linkedField_insert(cxa_linkedField_t *const fbbLfIn, const size_t indexIn, uint8_t *const ptrIn, const size_t numBytesIn);

size_t cxa_linkedField_getSize_bytes(cxa_linkedField_t *const fbbLfIn);
size_t cxa_linkedField_getMaxSize_bytes(cxa_linkedField_t *const fbbLfIn);
size_t cxa_linkedField_getFreeSize_bytes(cxa_linkedField_t *const fbbLfIn);
size_t cxa_linkedField_getStartIndexInParent(cxa_linkedField_t *const fbbLfIn);

#endif // CXA_LINKED_FIELD_H_
