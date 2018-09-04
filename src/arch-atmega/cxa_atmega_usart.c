/**
a * Copyright 2018 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "cxa_atmega_usart.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdbool.h>
#include <cxa_assert.h>
#include <cxa_delay.h>
#include <cxa_numberUtils.h>
#include <uart.h>


#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define RX_RING_BUFFER_SIZE_BYTES			2048


// ******** local type definitions ********


// ******** local function prototypes ********
static cxa_ioStream_readStatus_t ioStream_cb_readByte(uint8_t *const byteOut, void *const userVarIn);
static bool ioStream_cb_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_atmega_usart_init_noHH(cxa_atmega_usart_t *const usartIn, cxa_atmega_usart_id_t usartIdIn, const uint32_t baudRate_bpsIn)
{
	cxa_assert(usartIdIn == CXA_ATM_USART_ID_0);

	uart0_init(UART_BAUD_SELECT_DOUBLE_SPEED(baudRate_bpsIn, F_CPU));

	// setup our ioStream (last once everything is setup)
	cxa_ioStream_init(&usartIn->super.ioStream);
	cxa_ioStream_bind(&usartIn->super.ioStream, ioStream_cb_readByte, ioStream_cb_writeBytes, (void*)usartIn);
}


// ******** local function implementations ********
static cxa_ioStream_readStatus_t ioStream_cb_readByte(uint8_t *const byteOut, void *const userVarIn)
{
	cxa_atmega_usart_t* usartIn = (cxa_atmega_usart_t*)userVarIn;
	cxa_assert(usartIn);

	cxa_ioStream_readStatus_t retVal = CXA_IOSTREAM_READSTAT_ERROR;

	uint16_t rxVal = uart0_getc();
	switch( ((rxVal >> 8) & 0x00FF) )
	{
		case 0:
		case UART_BUFFER_OVERFLOW:
		case UART_OVERRUN_ERROR:
			// data received..even if we lost some along the way...
			if( byteOut != NULL ) *byteOut = rxVal & 0x00FF;
			retVal = CXA_IOSTREAM_READSTAT_GOTDATA;
			break;

		default:
			// no data to receive
			retVal = CXA_IOSTREAM_READSTAT_NODATA;
			break;
	}

	return retVal;
}


static bool ioStream_cb_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn)
{
	cxa_atmega_usart_t* usartIn = (cxa_atmega_usart_t*)userVarIn;
	cxa_assert(usartIn);

	for( size_t i = 0; i < bufferSize_bytesIn; i++ )
	{
		uart0_putc(((uint8_t*)buffIn)[i]);
	}

	return true;
}
