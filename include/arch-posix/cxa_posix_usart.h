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
 * @copyright 2013-20145 opencxa.org
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
#ifndef CXA_POSIX_USART_H_
#define CXA_POSIX_USART_H_


// ******** includes ********
#include <stdio.h>
#include <stdbool.h>
#include <termios.h>
#include <cxa_config.h>
#include <cxa_usart.h>
#include <cxa_fixedFifo.h>
#include <cxa_gpio.h>


// ******** global macro definitions ********
#ifndef CXA_POSIX_USART_RX_FIFO_SIZE_BYTES
	#define CXA_POSIX_USART_RX_FIFO_SIZE_BYTES			8
#endif


// ******** global type definitions *********
/**
 * @private
 */
struct cxa_usart
{
	int fd;
};


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes the specified serial port for no hardware handshaking using the specified baud rate.
 * Defaults to 8n1 for databits / parity / stopbits respectively.
 *
 * @param[in] usartIn pointer to a pre-allocated USART object
 * @param[in] pathIn path to the target UART file device (eg. /dev/ttyUSB0)
 * @param[in] baudRateIn the desired baud rate as specified in termios.h (eg. B115200)
 *
 * @return true if successfully opened, false if not
 */
bool cxa_posix_usart_init_noHH(cxa_usart_t *const usartIn, char *const pathIn, const int baudRateIn);


/**
 * Closes the specified serial port.
 *
 * @param[in] usartIn pointer to the pre-initialized serial port to close
 */
void cxa_posix_usart_close(cxa_usart_t *const usartIn);


#endif // CXA_POSIX_USART_H_
