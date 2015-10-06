/**
 * @file
 *
 *
 * #### Example Usage: ####
 *
 * @code
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
#ifndef CXA_BLE112_USART_H_
#define CXA_BLE112_USART_H_


// ******** includes ********
#include <stdio.h>
#include <stdint.h>
#include <cxa_config.h>
#include <cxa_usart.h>
#include <cxa_fixedFifo.h>
#include <cxa_gpio.h>


// ******** global macro definitions ********
#ifndef CXA_BLE112_USART_RX_FIFO_SIZE_BYTES
	#define CXA_BLE112_USART_RX_FIFO_SIZE_BYTES			16
#endif


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_ble112_usart_t object
 */
typedef struct cxa_ble112_usart cxa_ble112_usart_t;


typedef enum
{
	CXA_BLE112_USART_0,
	CXA_BLE112_USART_1,
}cxa_ble112_usart_id_t;


typedef enum
{
	CXA_BLE112_USART_PINCONFIG_ALT1,
	CXA_BLE112_USART_PINCONFIG_ALT2,
}cxa_ble112_usart_pinConfig_t;


typedef enum
{
	CXA_BLE112_USART_BAUD_2400,
	CXA_BLE112_USART_BAUD_4800,
	CXA_BLE112_USART_BAUD_9600,
	CXA_BLE112_USART_BAUD_14400,
	CXA_BLE112_USART_BAUD_19200,
	CXA_BLE112_USART_BAUD_28800,
	CXA_BLE112_USART_BAUD_38400,
	CXA_BLE112_USART_BAUD_57600,
	CXA_BLE112_USART_BAUD_76800,
	CXA_BLE112_USART_BAUD_115200,
	CXA_BLE112_USART_BAUD_230400,
}cxa_ble112_usart_baudRate_t;


/**
 * @private
 */
struct cxa_ble112_usart
{
	cxa_usart_t super;

	cxa_ble112_usart_id_t id;

	cxa_fixedFifo_t rxFifo;
	uint8_t rxFifo_raw[CXA_BLE112_USART_RX_FIFO_SIZE_BYTES];
};


// ******** global function prototypes ********
void cxa_ble112_usart_init_noHH(cxa_ble112_usart_t *const usartIn, const cxa_ble112_usart_id_t idIn, const cxa_ble112_usart_pinConfig_t pinConfigIn, const cxa_ble112_usart_baudRate_t baudRateIn);

#endif // CXA_BLE112_USART_H_
