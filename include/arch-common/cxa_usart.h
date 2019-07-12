/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */

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
 */
#ifndef CXA_USART_H_
#define CXA_USART_H_


// ******** includes ********
#include <stdio.h>
#include <stdbool.h>
#include <cxa_ioStream.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct
{
	cxa_ioStream_t ioStream;
}cxa_usart_t;


// ******** global function prototypes ********
/**
 * @public
 */
cxa_ioStream_t* cxa_usart_getIoStream(cxa_usart_t* usartIn);


#endif // CXA_USART_H_
