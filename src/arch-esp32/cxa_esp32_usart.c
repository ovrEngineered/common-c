/**
a * Copyright 2016 opencxa.org
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
#include "cxa_esp32_usart.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdbool.h>
#include <cxa_assert.h>
#include <cxa_delay.h>
#include <cxa_numberUtils.h>


#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define RX_RING_BUFFER_SIZE_BYTES			2048


// ******** local type definitions ********


// ******** local function prototypes ********
static void commonInit(cxa_esp32_usart_t *const usartIn, uart_port_t uartIdIn, const uint32_t baudRate_bpsIn,
						bool useHardwareHandshakingIn,
						const gpio_num_t txPinIn, const gpio_num_t rxPinIn,
						const gpio_num_t rtsPinIn, const gpio_num_t ctsPinIn);
static cxa_ioStream_readStatus_t ioStream_cb_readByte(uint8_t *const byteOut, void *const userVarIn);
static bool ioStream_cb_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_esp32_usart_init_noHH(cxa_esp32_usart_t *const usartIn, uart_port_t uartIdIn, const uint32_t baudRate_bpsIn,
						  const gpio_num_t txPinIn, const gpio_num_t rxPinIn)
{
	commonInit(usartIn, uartIdIn, baudRate_bpsIn, false, txPinIn, rxPinIn, 0, 0);
}


void cxa_esp32_usart_init_HH(cxa_esp32_usart_t *const usartIn, uart_port_t uartIdIn, const uint32_t baudRate_bpsIn,
							const gpio_num_t txPinIn, const gpio_num_t rxPinIn,
							const gpio_num_t rtsPinIn, const gpio_num_t ctsPinIn)
{
	commonInit(usartIn, uartIdIn, baudRate_bpsIn, true, txPinIn, rxPinIn, rtsPinIn, ctsPinIn);
}


// ******** local function implementations ********
static void commonInit(cxa_esp32_usart_t *const usartIn, uart_port_t uartIdIn, const uint32_t baudRate_bpsIn,
						bool useHardwareHandshakingIn,
						const gpio_num_t txPinIn, const gpio_num_t rxPinIn,
						const gpio_num_t rtsPinIn, const gpio_num_t ctsPinIn)
{
	cxa_assert(usartIn);
	cxa_assert( (uartIdIn == UART_NUM_0) ||
				(uartIdIn == UART_NUM_1) ||
				(uartIdIn == UART_NUM_2) );

	// save our references
	usartIn->uartId = uartIdIn;

	// setup our UART hardware
	uart_config_t uart_config = {
			.baud_rate = baudRate_bpsIn,
			.data_bits = UART_DATA_8_BITS,
			.parity = UART_PARITY_DISABLE,
			.stop_bits = UART_STOP_BITS_1,
			.flow_ctrl = useHardwareHandshakingIn ? UART_HW_FLOWCTRL_CTS_RTS : UART_HW_FLOWCTRL_DISABLE,
			.rx_flow_ctrl_thresh = 120
	};
	uart_param_config(usartIn->uartId, &uart_config);

	cxa_assert(uart_driver_install(usartIn->uartId, RX_RING_BUFFER_SIZE_BYTES, 0, 10, NULL, 0) == ESP_OK);
	cxa_assert(uart_set_pin(usartIn->uartId,
			txPinIn, rxPinIn,
			(useHardwareHandshakingIn ? rtsPinIn : UART_PIN_NO_CHANGE),
			(useHardwareHandshakingIn ? ctsPinIn : UART_PIN_NO_CHANGE) ) == ESP_OK);

	// flush the uart
	uart_flush(usartIn->uartId);

	// setup our ioStream (last once everything is setup)
	cxa_ioStream_init(&usartIn->super.ioStream);
	cxa_ioStream_bind(&usartIn->super.ioStream, ioStream_cb_readByte, ioStream_cb_writeBytes, (void*)usartIn);
}


static cxa_ioStream_readStatus_t ioStream_cb_readByte(uint8_t *const byteOut, void *const userVarIn)
{
	cxa_esp32_usart_t* usartIn = (cxa_esp32_usart_t*)userVarIn;
	cxa_assert(usartIn);
	
	uint8_t tmpBuff;

	cxa_ioStream_readStatus_t retVal = CXA_IOSTREAM_READSTAT_ERROR;
	switch( uart_read_bytes(usartIn->uartId, &tmpBuff, 1, 0) )
	{
		case 0:
			retVal = CXA_IOSTREAM_READSTAT_NODATA;
			break;

		case 1:
			if( byteOut != NULL ) *byteOut = tmpBuff;
			retVal = CXA_IOSTREAM_READSTAT_GOTDATA;
			break;

		default:
			// error
			break;
	}

	return retVal;
}


static bool ioStream_cb_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn)
{
	cxa_esp32_usart_t* usartIn = (cxa_esp32_usart_t*)userVarIn;
	cxa_assert(usartIn);

	uart_write_bytes(usartIn->uartId, buffIn, bufferSize_bytesIn);

	return true;
}
