/**
 * Copyright 2016 opencxa.org
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
#include "cxa_awcu300_usart.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdbool.h>
#include <cxa_assert.h>
#include <cxa_delay.h>

#include <mw300_pinmux.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static cxa_ioStream_readStatus_t ioStream_cb_readByte(uint8_t *const byteOut, void *const userVarIn);
static bool ioStream_cb_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_awcu300_usart_init_noHH(cxa_awcu300_usart_t *const usartIn, UART_ID_Type uartIdIn, const uint32_t baudRate_bpsIn)
{
	cxa_assert(usartIn);
	cxa_assert( (uartIdIn == UART0_ID) ||
				(uartIdIn == UART1_ID) ||
				(uartIdIn == UART2_ID) );

	// save our references
	usartIn->uartId = uartIdIn;

	// configure the UART hardware
	UART_CFG_Type uartCfg =
			{
				.baudRate = baudRate_bpsIn,
				.dataBits = UART_DATABITS_8,
				.highSpeedUart = ENABLE,
				.nrzCodeing = DISABLE,
				.parity = UART_PARITY_NONE,
				.stickyParity = DISABLE
			};
	UART_FifoCfg_Type fifoCfg =
			{
				.autoFlowControl = DISABLE,
				.fifoDmaEnable = DISABLE,
				.fifoEnable = ENABLE,
				.peripheralBusType = UART_PERIPHERAL_BITS_8,
				.rxFifoLevel = UART_RXFIFO_BYTE_1,
				.txFifoLevel = UART_TXFIFO_EMPTY,
			};
	UART_Disable(usartIn->uartId);
	UART_Enable(usartIn->uartId);
	UART_Init(usartIn->uartId, &uartCfg);
	UART_SetStopBits(usartIn->uartId, UART_STOPBITS_1);
	UART_FifoConfig(usartIn->uartId, &fifoCfg);

	// further configuration depends on which UART we're working with...
	switch( usartIn->uartId )
	{
		case UART0_ID:
			GPIO_PinMuxFun(GPIO_2, GPIO2_UART0_TXD);
			GPIO_PinMuxFun(GPIO_3, GPIO3_UART0_RXD);
			break;

		case UART1_ID:
			GPIO_PinMuxFun(GPIO_13, GPIO13_UART1_TXD);
			GPIO_PinMuxFun(GPIO_14, GPIO14_UART1_RXD);
			break;

		case UART2_ID:
			GPIO_PinMuxFun(GPIO_48, GPIO48_UART2_TXD);
			GPIO_PinMuxFun(GPIO_49, GPIO49_UART2_RXD);
			break;
	}

	// setup our ioStream (last once everything is setup)
	cxa_ioStream_init(&usartIn->super.ioStream);
	cxa_ioStream_bind(&usartIn->super.ioStream, ioStream_cb_readByte, ioStream_cb_writeBytes, (void*)usartIn);

	// let the io lines settle out
	cxa_delay_ms(500);
}


// ******** local function implementations ********
static cxa_ioStream_readStatus_t ioStream_cb_readByte(uint8_t *const byteOut, void *const userVarIn)
{
	cxa_awcu300_usart_t* usartIn = (cxa_awcu300_usart_t*)userVarIn;
	cxa_assert(usartIn);
	
	if( UART_GetRxFIFOLevel(usartIn->uartId) == 0 ) return CXA_IOSTREAM_READSTAT_NODATA;
	uint32_t rxByte = UART_ReceiveData(usartIn->uartId);
	if( byteOut != NULL ) *byteOut = (uint8_t)rxByte;
	return CXA_IOSTREAM_READSTAT_GOTDATA;
}


static bool ioStream_cb_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn)
{
	cxa_awcu300_usart_t* usartIn = (cxa_awcu300_usart_t*)userVarIn;
	cxa_assert(usartIn);

	for( size_t i = 0; i < bufferSize_bytesIn; i++ )
	{
		while( UART_GetLineStatus(usartIn->uartId, UART_LINESTATUS_TDRQ) != SET );
		UART_SendData(usartIn->uartId, ((char*)buffIn)[i]);
	}

	return true;
}
