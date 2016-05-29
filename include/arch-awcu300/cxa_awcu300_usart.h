/**
 * @file
 * @copyright 2016 opencxa.org
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
#ifndef CXA_AWCU300_USART_H_
#define CXA_AWCU300_USART_H_


// ******** includes ********
#include <stdio.h>
#include <cxa_config.h>
#include <cxa_usart.h>
#include <cxa_fixedFifo.h>
#include <cxa_gpio.h>

#include <mw300_uart.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_awcu300_usart_t object
 */
typedef struct cxa_awcu300_usart cxa_awcu300_usart_t;


/**
 * @private
 */
struct cxa_awcu300_usart
{
	cxa_usart_t super;
	
	UART_ID_Type uartId;
};


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes the specified USART for no hardware handshaking using the specified baud rate.
 *
 * @param[in] usartIn pointer to a pre-allocated USART object
 * @param[in] uartIdIn the UARTID specifying the desired UART
 * @param[in] baudRate_bpsIn the desired baud rate, in bits-per-second
 */
void cxa_awcu300_usart_init_noHH(cxa_awcu300_usart_t *const usartIn, UART_ID_Type uartIdIn, const uint32_t baudRate_bpsIn);


#endif // CXA_AWCU300_USART_H_
