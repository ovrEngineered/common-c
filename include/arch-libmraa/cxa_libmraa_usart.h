/**
 * @file
 * This file contains prototypes and an architecture-specific implementation of a 
 * Universal Serial Asynchronous Receiver/Transmitter (USART) object.
 *
 * @note This file contains functionality restricted to the CXA POSIX implementation.
 *
 * @note This file contains functionality in addition to that already provided in @ref cxa_usart.h
 *		
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * cxa_posix_usart_t myUsart;
 * // initialize the usart on '/dev/cu.usbserial-FTE3XG86' (nonblocking in OSX) for no hardware
 * // handshaking with a baud rate of 115200bps
 * cxa_posix_usart_init_noHH(&usart, "/dev/cu.usbserial-FTE3XG86", B115200);
 *
 * // read byte
 * uint8_t myByte;
 * cxa_usart_read(&usart, (void*)&myByte, 1, NULL);
 * @endcode
 *
 *
 * @copyright 2013-2015 opencxa.org
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
#ifndef CXA_LIBMRAA_USART_H_
#define CXA_LIBMRAA_USART_H_


// ******** includes ********
#include <stdio.h>
#include <stdbool.h>
#include <cxa_config.h>
#include <cxa_usart.h>
#include <mraa.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct
{
	cxa_usart_t super;

	mraa_uart_context mraaUart;
}cxa_libmraa_usart_t;


// ******** global function prototypes ********
bool cxa_libmraa_usart_init_noHH(cxa_libmraa_usart_t *const usartIn, int uartIndexIn, unsigned int baudRate_bpsIn);
void cxa_libmraa_usart_close(cxa_libmraa_usart_t *const usartIn);


#endif // CXA_LIBMRAA_USART_H_
