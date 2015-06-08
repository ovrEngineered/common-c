/**
 * @file
 * This file contains prototypes and a top-level implementation of a Universal Serial Asynchronous Receiver/Transmitter
 * (USART) object. A USART object is used to send and receive serial data via architecture-specified implementations
 * of a USART port (may be dedicated hardware, or bit-bang software).
 *
 * Once initialized, most serial input/output (I/O) is done using a file descriptor returned by ::cxa_usart_getFileDescriptor.
 * Most embedded libcs have support for a "virtual" file descriptor (newlib: fopen_cookie, avrlibc: fdev_setup_stream).
 * These "virtual" file descriptors route serial data to/from internal callbacks to actually send/receive data using the 
 * underlying implementation. This option was chosen because of the inherent added functionality of features like printf, 
 * scanf, fgetc, and fputc. <b>All serial I/O should be performed using the returned file descriptor.</b>
 *
 * @note This file contains the base functionality for a USART object available across all architectures. Additional
 *		functionality, including initialization is available in the architecture-specific implementation.
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * cxa_<arch>_usart_t myUsart;
 * // initialization for architecture specific implementation
 *
 * ...
 *
 * FILE *fd_usart = cxa_usart_getFileDescriptor(&myUsart);
 *
 * // send string 'foo' to serial port
 * fputs("foo", fd_usart);
 *
 * // read bytes (EOF is returned when no data is available...it is OK to keep reading)
 * while(1)
 * {
 *    int bar = fgetc(fd_usart);
 *    if( bar != EOF )
 *    {
 *       // received a byte!
 *    }
 * }
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
#ifndef CXA_USART_H_
#define CXA_USART_H_


// ******** includes ********
#include <stdio.h>
#include <stdbool.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_gpio_t object
 * (fully opaque type)
 */
typedef struct cxa_usart cxa_usart_t;


// ******** global function prototypes ********
/**
 * @public
 * @brief Attempts to read up to countIn bytes from the usart into the specified
 * buffer
 *
 * @param[in] usartIn pointer to a pre-initialized USART object
 * @param[in] buffIn pointer to a memory location at which to store
 * 		any read bytes
 * @param[in] desiredReadSize_bytesIn the maximum number of bytes that should
 * 		be copied to buffIn before returning. Must be <= SSIZE_MAX.
 * @param[in] actualReadSize_bytesOut the _actual_ number of bytes read into
 * 		buffIn. If enough data is available, will be set to desiredReadSizeIn,
 * 		otherwise will be set to number of bytes actually read.
 *
 * @return true if 0 - desiredReadSize_bytesIn bytes were read, false if there
 * 		was an error with the underlying USART.
 */
bool cxa_usart_read(cxa_usart_t* usartIn, void* buffIn, size_t desiredReadSize_bytesIn, size_t* actualReadSize_bytesOut);


/**
 * @public
 * @brief Writes the specified bytes to the USART.
 *
 * @note On some implementations, this function will block until all data
 * has been written to the USART. On some implementations, this function
 * will queue data to be sent at some time later.
 *
 * @param[in] usartIn pointer to a pre-initialized USART object
 * @param[in] buffIn pointer to a memory location which contains the bytes
 * 		to write
 * @param[in] bufferSizeIn the number of bytes from buffIn to write.
 * 		Must be <= SSIZE_MAX.
 *
 * @return true if all bytes were sent / queued to be sent, false if there
 * 		was an error with the underlying USART. If false, number of bytes
 * 		queued or sent is undetermined.
 */
bool cxa_usart_write(cxa_usart_t* usartIn, void* buffIn, size_t bufferSize_bytesIn);


/**
 * @public
 * @brief Returns a file descriptor which should be used for all serial I/O.
 * The file descriptor _may_ use an underlying buffer scheme. Frequent use
 * of fflush() is highly suggested.
 *
 * @note When reading from the file descriptor, fgetc() or similar may return
 *		EOF. This is normal and indicates that no serial data has been received.
 *		It is standard practice to continue reading from the file descriptor
 *		until an EOF is no longer received.
 *
 * @param[in] superIn pointer to a pre-initialized USART object
FILE* cxa_usart_getFileDescriptor(cxa_usart_t *const superIn);
 */


#endif // CXA_USART_H_
