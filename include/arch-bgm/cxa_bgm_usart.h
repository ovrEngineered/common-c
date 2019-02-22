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
#ifndef CXA_BGM_USART_H_
#define CXA_BGM_USART_H_


// ******** includes ********
#include <stdbool.h>
#include <stdio.h>
#include <cxa_config.h>
#include <cxa_usart.h>
#include <cxa_fixedFifo.h>
#include <cxa_gpio.h>

#include <em_usart.h>
#include <em_gpio.h>


// ******** global macro definitions ********
#ifndef CXA_BGM_USART_RX_FIFO_SIZE_BYTES
#define CXA_BGM_USART_RX_FIFO_SIZE_BYTES		16
#endif


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_bgm_usart_t object
 */
typedef struct cxa_bgm_usart cxa_bgm_usart_t;


/**
 * @private
 */
struct cxa_bgm_usart
{
	cxa_usart_t super;
	
	USART_TypeDef* uartId;

	cxa_fixedFifo_t fifo_rx;
	uint8_t fifo_rx_raw[CXA_BGM_USART_RX_FIFO_SIZE_BYTES];
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
void cxa_bgm_usart_init_noHH(cxa_bgm_usart_t *const usartIn, USART_TypeDef* uartIdIn,
							 const uint32_t baudRate_bpsIn,
							 const GPIO_Port_TypeDef txPortNumIn, const unsigned int txPinNumIn, uint32_t txLocIn,
							 const GPIO_Port_TypeDef rxPortNumIn, const unsigned int rxPinNumIn, uint32_t rxLocIn);

#endif
