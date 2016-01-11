/**
 * @file
 * @Note: the usart on the esp8266 is backed by a 127 byte fifo so we
 * 		don't actually need to respond to any interrupts...
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
#ifndef CXA_ESP8266_USART_H_
#define CXA_ESP8266_USART_H_


// ******** includes ********
#include <stdio.h>
#include <cxa_config.h>
#include <cxa_usart.h>
#include <cxa_fixedFifo.h>
#include <cxa_gpio.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_esp8266_usart_t object
 */
typedef struct cxa_esp8266_usart cxa_esp8266_usart_t;


typedef enum
{
	CXA_ESP8266_USART_0=0,
	CXA_ESP8266_USART_1=1,
	CXA_ESP8266_USART_0_ALTPINS=255
}cxa_esp8266_usartId_t;


/**
 * @private
 */
struct cxa_esp8266_usart
{
	cxa_usart_t super;

	cxa_esp8266_usartId_t id;
	uint32_t interCharDelay_ms;
};


// ******** global function prototypes ********
/**
 * @public
 */
void cxa_esp8266_usart_init_noHH(cxa_esp8266_usart_t *const usartIn, cxa_esp8266_usartId_t idIn, const uint32_t baudRate_bpsIn, uint32_t interCharDelay_msIn);


#endif // CXA_ESP8266_USART_H_
