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
#ifndef CXA_DUMMY_USART_H_
#define CXA_DUMMY_USART_H_


// ******** includes ********
#include <stdbool.h>
#include <stdio.h>
#include <cxa_config.h>
#include <cxa_usart.h>
#include <cxa_fixedFifo.h>
#include <cxa_gpio.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_dummy_usart_t object
 */
typedef struct cxa_dummy_usart cxa_dummy_usart_t;


/**
 * @private
 */
struct cxa_dummy_usart
{
	cxa_usart_t super;
};


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes the specified USART for no hardware handshaking using the specified baud rate.
 *
 * @param[in] usartIn pointer to a pre-allocated USART object
 * @param[in] baudRate_bpsIn the desired baud rate, in bits-per-second
 */
void cxa_dummy_usart_init_noHH(cxa_dummy_usart_t *const usartIn, const uint32_t baudRate_bpsIn);


#endif
