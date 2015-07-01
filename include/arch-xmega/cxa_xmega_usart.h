/**
 * @file
 * This file contains prototypes and an architecture-specific implementation of a 
 * Universal Serial Asynchronous Receiver/Transmitter (USART) object. This USART object
 * is used to send and receive serial data via dedicated hardware on the XMega processor.
 *
 * @note This file contains functionality restricted to the CXA Atmel XMega implementation.
 *
 * @note This file contains functionality in addition to that already provided in @ref cxa_usart.h
 *		
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * cxa_xmega_usart_t myUsart;
 * // initialize the usart 0 on PORTC for no hardware handshaking with a baud rate of 115200bps
 * cxa_xmega_usart_init_noHH(&myUsart, &USARTC0, 115200);
 *
 * // now, reassign stdin, stdout, stderr using functionality from cxa_usart.h
 * stdout = stdin = stderr = cxa_usart_getFileDescriptor(&myUsart.super);
 *
 * // finally, print a message
 * printf("hello world\r\n");
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
#ifndef CXA_XMEGA_USART_H_
#define CXA_XMEGA_USART_H_


// ******** includes ********
#include <stdio.h>
#include <avr/io.h>
#include <cxa_config.h>
#include <cxa_usart.h>
#include <cxa_fixedFifo.h>
#include <cxa_gpio.h>


// ******** global macro definitions ********
#ifndef CXA_XMEGA_USART_RX_FIFO_SIZE_BYTES
	#define CXA_XMEGA_USART_RX_FIFO_SIZE_BYTES			8
#endif


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_xmega_usart_t object
 */
typedef struct cxa_xmega_usart cxa_xmega_usart_t;


/**
 * @private
 */
struct cxa_xmega_usart
{
	cxa_usart_t super;
	
	USART_t *avrUsart;
	
	cxa_fixedFifo_t rxFifo;
	uint8_t rxFifo_raw[CXA_XMEGA_USART_RX_FIFO_SIZE_BYTES];
	
	bool isHandshakingEnabled;
	cxa_gpio_t *cts;
	cxa_gpio_t *rts;
};


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes the specified USART for no hardware handshaking using the specified baud rate.
 *
 * @param[in] usartIn pointer to a pre-allocated USART object
 * @param[in] avrUsartIn pointer to a USART_t struct that defines the hardware USART to be used (eg. USARTC0)
 * @param[in] baudRate_bpsIn the desired baud rate, in bits-per-second
 */
void cxa_xmega_usart_init_noHH(cxa_xmega_usart_t *const usartIn, USART_t *avrUsartIn, const uint32_t baudRate_bpsIn);


/**
 * @public
 * @brief Initializes the specified USART for hardware handshaking using the specified baud rate.
 * Hardware handshaking is a form of flow control that is achieved using RTS and CTS GPIO objects as follows:
 *     * RTS - output - electrical '0' it is OK to send data to me, electrical '1' do NOT send data to me
 *     * CTS - input - electrical '0' it is OK to to send data to remote device, electrical '1' do NOT send data to remote device
 *
 * @note This function will automatically configure the directions and polarity of RTS and CTS GPIO lines
 *
 * #### Example Usage: ####
 *
 * @code
 * cxa_xmega_usart_t myUsart;
 * cxa_xmega_gpio_t gpio_rts;
 * cxa_xmega_gpio_t gpio_cts;
 *
 * // initialize our gpios without changing their state/direction/polarity
 * // (will be set by cxa_xmega_usart_init_HH)
 * cxa_xmega_gpio_init_safe(&gpio_rts, &PORTE, 0);
 * cxa_xmega_gpio_init_safe(&gpio_cts, &PORTE, 1);
 *
 * // now initialize our USART
 * cxa_xmega_usart_init_HH(&myUsart, &USARTE0, 9600, &gpio_rts.super, &gpio_cts.super);
 * @endcode
 *
 * @param[in] usartIn pointer to a pre-allocated USART object
 * @param[in] avrUsartIn pointer to a USART_t struct that defines the hardware USART to be used (eg. USARTC0)
 * @param[in] baudRate_bpsIn the desired baud rate, in bits-per-second
 * @param[in] rtsIn pointer to the "safe" configured GPIO line that will serve as the output (detailed above)
 * @param[in] ctsIn pointer to the "safe" configured GPIO line that will serve as the input (detailed above)
 */
void cxa_xmega_usart_init_HH(cxa_xmega_usart_t *const usartIn, USART_t *avrUsartIn, const uint32_t baudRate_bpsIn,
	cxa_gpio_t *const rtsIn, cxa_gpio_t *const ctsIn);


#endif // CXA_XMEGA_USART_H_
