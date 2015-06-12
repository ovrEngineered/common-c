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
 * @brief Defined parameter that can be passed as the
 *		{@code length_BytesIn } parameter to either
 *		::cxa_fixedByteBuffer_init_subBuffer
 */
#define CXA_FIXED_BYTE_BUFFER_LEN_ALL		(SIZE_MAX-1)


/**
 * @public
 * @brief Maximum number of buffers that can be added to
 * 		a buffer via ::cxa_fixedByteBuffer_init_subBuffer
 */
#ifndef CXA_FIXED_BYTE_BUFFER_MAX_NUM_SUBBUFFERS
	#define CXA_FIXED_BYTE_BUFFER_MAX_NUM_SUBBUFFERS			5
#endif


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
	cxa_fixedByteBuffer_t *parent;
	size_t subBuff_startIndexInParent;
	size_t subBuff_maxSize_bytes;
	
	cxa_array_t subBuffers;
	cxa_fixedByteBuffer_t* subBuffers_raw[CXA_FIXED_BYTE_BUFFER_MAX_NUM_SUBBUFFERS];

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


/**
 * @public
 *
 * @param[in] maxSize_byteIn desired maximum size of the sub-buffer (max of SIZE_MAX-1),
 * 		CXA_FIXED_BYTE_BUFFER_LEN_ALL for full capacity of parent (minus startIndex)
 *
 * @return true if successfully initialized, false if not (likely due to a size / index issue)
 */
bool cxa_fixedByteBuffer_init_subBuffer(cxa_fixedByteBuffer_t *const subFbbIn, cxa_fixedByteBuffer_t *const parentFbbIn,
	size_t startIndexIn, size_t maxSize_bytesIn);


/**
 * @public
 * @brief Determines the current number of bytes in the buffer
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 *
 * @return the number of bytes in the given buffer
 */
size_t cxa_fixedByteBuffer_getCurrSize(cxa_fixedByteBuffer_t *const fbbIn);


/**
 * @public
 * @brief Determines the maximum number of bytes that can be contained within this buffer
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 *
 * @return the maximum number of bytes that can be contained in this buffer
 */
size_t cxa_fixedByteBuffer_getMaxSize(cxa_fixedByteBuffer_t *const fbbIn);


/**
 * @public
 * @brief Determines the number of unused bytes remaining in the buffer
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 *
 * @return the number of unused/free bytes in the buffer
 */
size_t cxa_fixedByteBuffer_getBytesRemaining(cxa_fixedByteBuffer_t *const fbbIn);


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
 * @brief Determines if this buffer is currently full (::cxa_fixedByteBuffer_getCurrSize == ::cxa_fixedByteBuffer_getMaxSize)
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 *
 * @return true if this buffer is full, false if not
 */
bool cxa_fixedByteBuffer_isFull(cxa_fixedByteBuffer_t *const fbbIn);


/**
 * @public
 * @brief Tries to append a single byte to the end of this buffer
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 * @param[in] byteIn the byte to append
 *
 * @return true if the byte was successfully appended, false if not (eg. buffer was full)
 */
bool cxa_fixedByteBuffer_append_uint8(cxa_fixedByteBuffer_t *const fbbIn, uint8_t byteIn);


/**
 * @public
 * @brief Tries to append a 16-bit unsigned integer to the end of this buffer in
 *		little-endian format (lower order byte first)
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 * @param[in] uint16In the unsigned integer to append
 *
 * @return true if the value was successfully appended, false if not (eg. buffer was full)
 */
bool cxa_fixedByteBuffer_append_uint16LE(cxa_fixedByteBuffer_t *const fbbIn, uint16_t uint16In);


/**
 * @public
 * @brief Tries to append a 32-bit unsigned integer to the end of this buffer in
 *		little-endian format (lower order byte first)
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 * @param[in] uint32In the unsigned integer to append
 *
 * @return true if the value was successfully appended, false if not (eg. buffer was full)
 */
bool cxa_fixedByteBuffer_append_uint32LE(cxa_fixedByteBuffer_t *const fbbIn, uint32_t uint32In);


/**
 * @public
 * @brief Tries to append a single-precision floating point number to the end of this buffer in
 *		little-endian format (lower order byte first).
 * @todo "little-endian" doesn't really apply here...figure out a better term
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 * @param[in] floatIn the floating point number to append
 *
 * @return true if the value was successfully appended, false if not (eg. buffer was full)
 */
bool cxa_fixedByteBuffer_append_floatLE(cxa_fixedByteBuffer_t *const fbbIn, float floatIn);


/**
 * @public
 * @brief Tries to append a null-terminated C String to the end of this buffer.
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 * @param[in] stringIn pointer to the null-terminated C String to append
 *
 * @return true if the value was successfully appended, false if not (eg. buffer was full)
 */
bool cxa_fixedByteBuffer_append_cString(cxa_fixedByteBuffer_t *const fbbIn, char *const stringIn);


/**
 * @public
 * @brief Tries to append all bytes contained within the source buffer to the end of the
 *		target buffer.
 * Any unused bytes from the buffer's maximum capacity will not be appended.
 *
 * @param[in] fbbIn pointer to the pre-initialized target buffer
 * @parma[in] srcFbbIn pointer to the pre-initialized source buffer
 *
 * @return true if the values were successfully appended, false if not (eg. buffer was full)
 */
bool cxa_fixedByteBuffer_append_fbb(cxa_fixedByteBuffer_t *const fbbIn, cxa_fixedByteBuffer_t *const srcFbbIn);


/**
 * @public
 * @brief returns a pointer to the byte at the provided index
 * @note Care should be taken to ensure that the provided index is less than
 *		::cxa_fixedByteBuffer_currSize. Indices that are out of bounds <b>will</b>
 *		generate an assert.
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 * @param[in] indexIn the index of the desired byte within the buffer
 *
 * @return pointer to the byte at the specified index
 */
uint8_t* cxa_fixedByteBuffer_get_pointerToIndex(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn);

/**
 * @public
 * @brief Returns the byte at the given index of the buffer.
 * @note Care should be taken to ensure that the provided index is less than
 *		::cxa_fixedByteBuffer_currSize. Indices that are out of bounds <b>will</b>
 *		generate an assert.
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 * @param[in] indexIn the index of the desired byte within the buffer
 *
 * @return the byte at the specified index
 */
uint8_t cxa_fixedByteBuffer_get_uint8(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn);


/**
 * @public
 * @brief Returns the 16-bit unsigned integer which starts at the given index of the buffer.
 * @note Care should be taken to ensure that 
 *		{@code (indexIn + 1) < cxa_array_getSize_elems(&fbbIn->bytes)}
 *		Indices that are out of bounds <b>will</b> generate an assert.
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 * @param[in] indexIn the index of the desired integer within the buffer
 *
 * @return the integer at the specified index
 */
uint16_t cxa_fixedByteBuffer_get_uint16LE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn);


/**
 * @public
 * @brief Returns the 32-bit unsigned integer which starts at the given index of the buffer.
 * @note Care should be taken to ensure that 
 *		{@code (indexIn + 3) < cxa_array_getSize_elems(&fbbIn->bytes)}
 *		Indices that are out of bounds <b>will</b> generate an assert.
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 * @param[in] indexIn the index of the desired integer within the buffer
 *
 * @return the integer at the specified index
 */
uint32_t cxa_fixedByteBuffer_get_uint32LE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn);


/**
 * @public
 * @brief Returns the single-precision floating point number which starts at the given index of the buffer.
 * @note Care should be taken to ensure that 
 *		{@code (indexIn + 3) < cxa_array_getSize_elems(&fbbIn->bytes)}
 *		Indices that are out of bounds <b>will</b> generate an assert.
 * @todo "little-endian" doesn't really apply here...figure out a better term
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 * @param[in] indexIn the index of the desired number within the buffer
 *
 * @return the number at the specified index
 */
float cxa_fixedByteBuffer_get_floatLE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn);


/**
 * @public
 * @brief Reads a null-terminated C String from the buffer starting at the specified index.
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 * @param[in] indexIn the index of the desired string within the buffer
 * @param[out] stringOut pointer to a buffer at which to store the string
 * @param[in] maxOutputSize_bytes the maximum number of bytes to write to 'stringOut'
 *
 * @return true if the entire string (including null terminator) was written to
 * 		stringOut, false if not (eg. maxOutputSize_bytes is too small OR null
 * 		terminator not found before end of fixed byte buffer)
 */
bool cxa_fixedByteBuffer_get_cString(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, char *const stringOut, size_t maxOutputSize_bytes);


/**
 * @public
 * @brief Tries to replace a single byte at the given index
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 * @param[in] indexIn the index of the desired value to replace
 * @param[in] byteIn the byte to append
 *
 * @return true if the value was successfully replaced, false if not (eg. buffer was full)
 */
bool cxa_fixedByteBuffer_overwrite_uint8(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, uint8_t byteIn);


/**
 * @public
 * @brief Tries to replace a uint16 starting at the given index in
 * little-endian format (lower order byte first)
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 * @param[in] indexIn the index of the desired value to replace
 * @param[in] byteIn the byte to append
 *
 * @return true if the value was successfully replaced, false if not (eg. buffer was full)
 */
bool cxa_fixedByteBuffer_overwrite_uint16LE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, uint16_t uint16In);


/**
 * @public
 * @brief Tries to replace a uint32 starting at the given index in
 * little-endian format (lower order byte first)
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 * @param[in] indexIn the index of the desired value to replace
 * @param[in] byteIn the byte to append
 *
 * @return true if the value was successfully replaced, false if not (eg. buffer was full)
 */
bool cxa_fixedByteBuffer_overwrite_uint32LE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, uint16_t uint32In);


/**
 * @public
 * @brief Tries to replace a float starting at the given index in
 * little-endian format (lower order byte first).
 * @todo "little-endian" doesn't really apply here...figure out a better term
 *
 * @param[in] fbbIn pointer to the pre-initialized fixedByteBuffer object
 * @param[in] indexIn the index of the desired value to replace
 * @param[in] byteIn the byte to append
 *
 * @return true if the value was successfully replaced, false if not (eg. buffer was full)
 */
bool cxa_fixedByteBuffer_overwrite_floatLE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, float floatIn);


bool cxa_fixedByteBuffer_insert_uint8(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, uint8_t valueIn);
bool cxa_fixedByteBuffer_insert_uint16LE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, uint16_t valueIn);
bool cxa_fixedByteBuffer_insert_uint32LE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, uint32_t valueIn);
bool cxa_fixedByteBuffer_insert_floatLE(cxa_fixedByteBuffer_t *const fbbIn, const size_t indexIn, float valueIn);

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
 */
void cxa_fixedByteBuffer_writeToFile_asciiHexRep(cxa_fixedByteBuffer_t *const fbbIn, FILE *fileIn);


#endif // CXA_FIXED_BYTE_BUFFER_H_
