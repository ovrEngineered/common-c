/**
 * @file
 *
 * @note This object should work across all architecture-specific implementations
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * cxa_fixedByteBuffer_t myBuffer;
 * uint8_t myBuffer_buffer[16];			// where the data is actually stored
 *
 * // initialize the buffer with a maximum of 16 elements
 * cxa_fixedByteBuffer_init(&myBuffer, (void*)myBuffer_buffer, sizeof(myBuffer_buffer));
 *
 * ...
 *
 * // add two new values to the buffer (0x47 and 0x65)
 * cxa_fixedByteBuffer_append_byte(&myBuffer, 0x47);
 * cxa_fixedByteBuffer_append_byte(&myBuffer, 0x65);
 *
 * ...
 *
 * // now retrieve the two values out of the buffer treating them as parts of a 16-bit integer
 * uint16_t foo = cxa_fixedByteBuffer_get_uint16LE(&myBuffer, 0);
 * // foo = 0x6547
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
#ifndef CXA_FIXED_BYTE_BUFFER_H_
#define CXA_FIXED_BYTE_BUFFER_H_


// ******** includes ********
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <cxa_array.h>


// ******** global macro definitions ********
#ifndef ssize_t
#define ssize_t long
#endif


/**
 * @public
 * @brief Shortcut to initialize a fixedByteBuffer with a buffer of an explicit data type
 *
 * @code
 * cxa_fixedByteBuffer_t myFbb;
 * double myBuffer[100];
 *
 * cxa_fixedByteBuffer_initStd(&myArray, myBuffer);
 * // equivalent to
 * cxa_fixedByteBuffer_init(&myFbb, (void)myBuffer, sizeof(myBuffer));
 * @endcode
 *
 * @param[in] fbbIn pointer to the pre-allocated fixedByteBuffer object
 * @param[in] bufferIn pointer to the declared c-style array which
 * 		will contain the data for the fixedByteBuffer
 */
#define cxa_fixedByteBuffer_initStd(fbbIn, bufferIn)						cxa_fixedByteBuffer_init((fbbIn), ((void*)(bufferIn)), sizeof(bufferIn))

#define cxa_fixedByteBuffer_append_uint8(fbbIn, uint8In)					cxa_fixedByteBuffer_append((fbbIn), (uint8_t[]){(uint8In)}, 1)
#define cxa_fixedByteBuffer_append_uint16LE(fbbIn, uint16In)				cxa_fixedByteBuffer_append((fbbIn), (uint8_t[]){((uint8_t)((((uint16_t)(uint16In)) & 0x00FF) >> 0)), ((uint8_t)((((uint16_t)(uint16In)) & 0xFF00) >> 8))}, 2)
#define cxa_fixedByteBuffer_append_uint32LE(fbbIn, uint32In)				cxa_fixedByteBuffer_append((fbbIn), (uint8_t[]){ ((uint8_t)((((uint32_t)(uint32In)) & 0x000000FF) >> 0)), ((uint8_t)((((uint32_t)(uint32In)) & 0x0000FF00) >> 8)), ((uint8_t)((((uint32_t)(uint32In)) & 0x00FF0000) >> 16)), ((uint8_t)((((uint32_t)(uint32In)) & 0xFF000000) >> 24)) }, 4)
#define cxa_fixedByteBuffer_append_float(fbbIn, floatIn)					cxa_fixedByteBuffer_append((fbbIn), (uint8_t*)&(floatIn), 4)
#define cxa_fixedByteBuffer_append_cString(fbbIn, strIn)					cxa_fixedByteBuffer_append((fbbIn), (uint8_t*)(strIn), (strlen(strIn)+1))

#define cxa_fixedByteBuffer_remove_uint8(fbbIn, indexIn)					cxa_fixedByteBuffer_remove((fbbIn), (indexIn), 1)
#define cxa_fixedByteBuffer_remove_uint16(fbbIn, indexIn)					cxa_fixedByteBuffer_remove((fbbIn), (indexIn), 2)
#define cxa_fixedByteBuffer_remove_uint32(fbbIn, indexIn)					cxa_fixedByteBuffer_remove((fbbIn), (indexIn), 4)
#define cxa_fixedByteBuffer_remove_float(fbbIn, indexIn)					cxa_fixedByteBuffer_remove((fbbIn), (indexIn), 4)

#define cxa_fixedByteBuffer_get_uint8(fbbIn, indexIn, uint8Out)				cxa_fixedByteBuffer_get((fbbIn), (indexIn), false, (uint8_t*)&(uint8Out), 1)
#define cxa_fixedByteBuffer_get_uint16LE(fbbIn, indexIn, uint16Out)			cxa_fixedByteBuffer_get((fbbIn), (indexIn), false, (uint8_t*)&(uint16Out), 2)
#define cxa_fixedByteBuffer_get_uint32LE(fbbIn, indexIn, uint32Out)			cxa_fixedByteBuffer_get((fbbIn), (indexIn), false, (uint8_t*)&(uint32Out), 4)
#define cxa_fixedByteBuffer_get_float(fbbIn, indexIn, floatOut)				cxa_fixedByteBuffer_get((fbbIn), (indexIn), false, (uint8_t*)&(floatOut), 4)

#define cxa_fixedByteBuffer_replace_uint8(fbbIn, indexIn, uint8In)			cxa_fixedByteBuffer_replace((fbbIn), (indexIn), (uint8_t[]){(uint8In)}, 1)
#define cxa_fixedByteBuffer_replace_uint16LE(fbbIn, indexIn, uint16In)		cxa_fixedByteBuffer_replace((fbbIn), (indexIn), (uint8_t[]){((uint8_t)((((uint16_t)(uint16In)) & 0x00FF) >> 0)), ((uint8_t)((((uint16_t)(uint16In)) & 0xFF00) >> 8))}, 2)
#define cxa_fixedByteBuffer_replace_uint32LE(fbbIn, indexIn, uint32In)		cxa_fixedByteBuffer_replace((fbbIn), (indexIn), (uint8_t[]){ ((uint8_t)((((uint32_t)(uint32In)) & 0x000000FF) >> 0)), ((uint8_t)((((uint32_t)(uint32In)) & 0x0000FF00) >> 8)), ((uint8_t)((((uint32_t)(uint32In)) & 0x00FF0000) >> 16)), ((uint8_t)((((uint32_t)(uint32In)) & 0xFF000000) >> 24)) }, 4)
#define cxa_fixedByteBuffer_replace_float(fbbIn, indexIn, floatIn)			cxa_fixedByteBuffer_replace((fbbIn), (indexIn), (uint8_t*)&(floatIn), 4)

#define cxa_fixedByteBuffer_insert_uint8(fbbIn, indexIn, uint8In)			cxa_fixedByteBuffer_insert((fbbIn), (indexIn), (uint8_t[]){(uint8In)}, 1)
#define cxa_fixedByteBuffer_insert_uint16(fbbIn, indexIn, uint16In)			cxa_fixedByteBuffer_insert((fbbIn), (indexIn), (uint8_t[]){((uint8_t)((((uint16_t)(uint16In)) & 0x00FF) >> 0)), ((uint8_t)((((uint16_t)(uint16In)) & 0xFF00) >> 8))}, 2)
#define cxa_fixedByteBuffer_insert_uint32(fbbIn, indexIn, uint32In)			cxa_fixedByteBuffer_insert((fbbIn), (indexIn), (uint8_t[]){ ((uint8_t)((((uint32_t)(uint32In)) & 0x000000FF) >> 0)), ((uint8_t)((((uint32_t)(uint32In)) & 0x0000FF00) >> 8)), ((uint8_t)((((uint32_t)(uint32In)) & 0x00FF0000) >> 16)), ((uint8_t)((((uint32_t)(uint32In)) & 0xFF000000) >> 24)) }, 4)
#define cxa_fixedByteBuffer_insert_float(fbbIn, indexIn, floatIn)			cxa_fixedByteBuffer_insert((fbbIn), (indexIn), (uint8_t*)&(floatIn), 4)
#define cxa_fixedByteBuffer_insert_cString(fbbIn, indexIn, strIn)			cxa_fixedByteBuffer_insert((fbbIn), (indexIn), (uint8_t*)(strIn), (strlen(strIn)+1))


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_fixedByteBuffer_t object
 */
typedef struct cxa_fixedByteBuffer cxa_fixedByteBuffer_t;


/**
 * @private
 */
struct cxa_fixedByteBuffer
{
	cxa_array_t bytes;
};


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes the pre-allocated fixedByteBuffer using the specified
 *		buffer to store elements.
 *
 * @param[in] fbbIn pointer to the pre-allocated fixedByteBuffer object
 * @param[in] bufferLocIn pointer to the pre-allocated chunk of memory that will
 * 		be used to store elements in the buffer
 * @param[in] bufferMaxSize_bytesIn the maximum size of the chunk of memory (buffer) in bytes
 */
void cxa_fixedByteBuffer_init(cxa_fixedByteBuffer_t *const fbbIn, void *const bufferLocIn, const size_t bufferMaxSize_bytesIn);


bool cxa_fixedByteBuffer_append(cxa_fixedByteBuffer_t *const fbbIn, uint8_t *const ptrIn, const size_t numBytesIn);
bool cxa_fixedByteBuffer_remove(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, const size_t numBytesIn);
bool cxa_fixedByteBuffer_remove_cString(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn);

uint8_t* cxa_fixedByteBuffer_get_pointerToIndex(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn);
bool cxa_fixedByteBuffer_get(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, bool transposeIn, uint8_t *const valOut, const size_t numBytesIn);
bool cxa_fixedByteBuffer_get_cString(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, char *const stringOut, size_t maxOutputSize_bytes);

bool cxa_fixedByteBuffer_replace(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, uint8_t *const ptrIn, const size_t numBytesIn);
bool cxa_fixedByteBuffer_replace_cString(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, char *const stringIn);

bool cxa_fixedByteBuffer_insert(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, uint8_t *const ptrIn, const size_t numBytesIn);


/**
 * @public
 * @brief Determines the current number of bytes in the buffer
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 *
 * @return the number of bytes in the given buffer
 */
size_t cxa_fixedByteBuffer_getSize_bytes(cxa_fixedByteBuffer_t *const fbbIn);


/**
 * @public
 * @brief Determines the maximum number of bytes that can be contained within this buffer
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 *
 * @return the maximum number of bytes that can be contained in this buffer
 */
size_t cxa_fixedByteBuffer_getMaxSize_bytes(cxa_fixedByteBuffer_t *const fbbIn);


/**
 * @public
 * @brief Determines the number of unused bytes remaining in the buffer
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 *
 * @return the number of unused/free bytes in the buffer
 */
size_t cxa_fixedByteBuffer_getFreeSize_bytes(cxa_fixedByteBuffer_t *const fbbIn);


/**
 * @public
 * @brief Determines if this buffer is currently full (::cxa_fixedByteBuffer_getCurrSize == ::cxa_fixedByteBuffer_getMaxSize)
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 *
 * @return true if this buffer is full, false if not
 */
bool cxa_fixedByteBuffer_isFull(cxa_fixedByteBuffer_t *const fbbIn);


/**
 * @public
 * @brief Determines if this buffer is currently empty (no bytes)
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 *
 * @return true if there are no bytes in this buffer, false if there are
 */
bool cxa_fixedByteBuffer_isEmpty(cxa_fixedByteBuffer_t *const fbbIn);


/**
 * @public
 * @brief Clears the buffer, discarding any elements current contained
 * within it. This function will result in an empty buffer, but the
 * underlying memory may still contain the original data.
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 */
void cxa_fixedByteBuffer_clear(cxa_fixedByteBuffer_t *const fbbIn);


/**
 * @public
 * @brief Writes the raw bytes contained within this buffer (excluding unused bytes)
 * to the file descriptor.
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 * @param[in] fileIn pointer to an open file descriptor to which the bytes will be written
 *
 * @return true on success, false on failure
 */
bool cxa_fixedByteBuffer_writeToFile_bytes(cxa_fixedByteBuffer_t *const fbbIn, FILE *fileIn);


/**
 * @public
 * @brief Writes a human-friendly representation of the bytes contained within this buffer (excluding unused bytes)
 * to the file descriptor.
 * <b>Example:</b>
 * @code {0x00, 0x01} @endcode
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 * @param[in] fileIn pointer to an open file descriptor to which the bytes will be written
 *
 * @return true on success, false on failure
 */
bool cxa_fixedByteBuffer_writeToFile_asciiHexRep(cxa_fixedByteBuffer_t *const fbbIn, FILE *fileIn);


#endif // CXA_FIXED_BYTE_BUFFER_H_
