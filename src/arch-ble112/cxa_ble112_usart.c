/**
 * Copyright 2013 opencxa.org
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
#include "cxa_ble112_usart.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdbool.h>
#include <cxa_assert.h>
#include <cxa_criticalSection.h>
#include <blestack/hw_regs.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static cxa_ioStream_readStatus_t ioStream_cb_readByte(uint8_t *const byteOut, void *const userVarIn);
static bool ioStream_cb_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn);

static void usart_setBaudRate(const cxa_ble112_usart_id_t portIdIn, const cxa_ble112_usart_baudRate_t baudRateIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_ble112_usart_init_noHH(cxa_ble112_usart_t *const usartIn, const cxa_ble112_usart_id_t idIn, const cxa_ble112_usart_pinConfig_t pinConfigIn, const cxa_ble112_usart_baudRate_t baudRateIn)
{
	cxa_assert(usartIn);

	// save our internal state
	usartIn->id = idIn;

	// setup our receive fifo
	cxa_fixedFifo_init(&usartIn->rxFifo, CXA_FF_ON_FULL_DROP, sizeof(*usartIn->rxFifo_raw), (void*)usartIn->rxFifo_raw, sizeof(usartIn->rxFifo_raw));

	// configure our port...
	if( (idIn == CXA_BLE112_USART_0) && (pinConfigIn == CXA_BLE112_USART_PINCONFIG_ALT1) )
	{
		PERCFG &= ~0x01;
		P0SEL |= 0x0C;
		U0CSR |= 0x80;
		UTX0IF = 0;
	}
	else if( (idIn == CXA_BLE112_USART_0) && (pinConfigIn == CXA_BLE112_USART_PINCONFIG_ALT2) )
	{
		PERCFG |= 0x01;
		P1SEL |= 0x30;
		U0CSR |= 0x80;
		UTX0IF = 0;
	}
	else if( (idIn == CXA_BLE112_USART_1) && (pinConfigIn == CXA_BLE112_USART_PINCONFIG_ALT1) )
	{
		PERCFG &= ~0x02;
		P0SEL |= 0x30;
		U1CSR |= 0x80;
		UTX1IF = 0;
	}
	else if( (idIn == CXA_BLE112_USART_1) && (pinConfigIn == CXA_BLE112_USART_PINCONFIG_ALT2) )
	{
		PERCFG |= 0x02;
		P0SEL |= 0xC0;
		U1CSR |= 0x80;
		UTX1IF = 0;
	}

	// set our baud rate
	usart_setBaudRate(idIn, baudRateIn);

	EA = 1;
	//URX1IE = 1;
	//IEN0 |= 0x84; // 1000 0100

	// setup our ioStream (last once everything is setup)
	cxa_ioStream_init(&usartIn->super.ioStream);
	cxa_ioStream_bind(&usartIn->super.ioStream, ioStream_cb_readByte, ioStream_cb_writeBytes, (void*)usartIn);
}


// ******** local function implementations ********
static void usart_setBaudRate(const cxa_ble112_usart_id_t portIdIn, const cxa_ble112_usart_baudRate_t baudRateIn)
{
	cxa_assert( (portIdIn == CXA_BLE112_USART_0) ||
				(portIdIn == CXA_BLE112_USART_1) );

	switch(baudRateIn)
	{
		case CXA_BLE112_USART_BAUD_2400:
			if( portIdIn == CXA_BLE112_USART_0 ) { U0BAUD = 59; U0GCR = (U0GCR & 0xE0) | 6; }
			else { U1BAUD = 59; U1GCR = (U1GCR & 0xE0) | 6; }
			break;

		case CXA_BLE112_USART_BAUD_4800:
			if( portIdIn == CXA_BLE112_USART_0 ) { U0BAUD = 59; U0GCR = (U0GCR & 0xE0) | 7; }
			else { U1BAUD = 59; U1GCR = (U1GCR & 0xE0) | 7; }
			break;

		case CXA_BLE112_USART_BAUD_9600:
			if( portIdIn == CXA_BLE112_USART_0 ) { U0BAUD = 59; U0GCR = (U0GCR & 0xE0) | 8; }
			else { U1BAUD = 59; U1GCR = (U1GCR & 0xE0) | 8; }
			break;

		case CXA_BLE112_USART_BAUD_14400:
			if( portIdIn == CXA_BLE112_USART_0 ) { U0BAUD = 216; U0GCR = (U0GCR & 0xE0) | 8; }
			else { U1BAUD = 216; U1GCR = (U1GCR & 0xE0) | 8; }
			break;

		case CXA_BLE112_USART_BAUD_19200:
			if( portIdIn == CXA_BLE112_USART_0 ) { U0BAUD = 59; U0GCR = (U0GCR & 0xE0) | 9; }
			else { U1BAUD = 59; U1GCR = (U1GCR & 0xE0) | 9; }
			break;

		case CXA_BLE112_USART_BAUD_28800:
			if( portIdIn == CXA_BLE112_USART_0 ) { U0BAUD = 216; U0GCR = (U0GCR & 0xE0) | 9; }
			else { U1BAUD = 216; U1GCR = (U1GCR & 0xE0) | 9; }
			break;

		case CXA_BLE112_USART_BAUD_38400:
			if( portIdIn == CXA_BLE112_USART_0 ) { U0BAUD = 59; U0GCR = (U0GCR & 0xE0) | 10; }
			else { U1BAUD = 59; U1GCR = (U1GCR & 0xE0) | 10; }
			break;

		case CXA_BLE112_USART_BAUD_57600:
			if( portIdIn == CXA_BLE112_USART_0 ) { U0BAUD = 216; U0GCR = (U0GCR & 0xE0) | 10; }
			else { U1BAUD = 216; U1GCR = (U1GCR & 0xE0) | 10; }
			break;

		case CXA_BLE112_USART_BAUD_76800:
			if( portIdIn == CXA_BLE112_USART_0 ) { U0BAUD = 59; U0GCR = (U0GCR & 0xE0) | 11; }
			else { U1BAUD = 59; U1GCR = (U1GCR & 0xE0) | 11; }
			break;

		case CXA_BLE112_USART_BAUD_115200:
			if( portIdIn == CXA_BLE112_USART_0 ) { U0BAUD = 216; U0GCR = (U0GCR & 0xE0) | 11; }
			else { U1BAUD = 216; U1GCR = (U1GCR & 0xE0) | 11; }
			break;

		case CXA_BLE112_USART_BAUD_230400:
			if( portIdIn == CXA_BLE112_USART_0 ) { U0BAUD = 216; U0GCR = (U0GCR & 0xE0) | 12; }
			else { U1BAUD = 216; U1GCR = (U1GCR & 0xE0) | 12; }
			break;
	}
}

static cxa_ioStream_readStatus_t ioStream_cb_readByte(uint8_t *const byteOut, void *const userVarIn)
{
	cxa_ble112_usart_t* usartIn = (cxa_ble112_usart_t*)userVarIn;
	cxa_assert(usartIn);

	return cxa_fixedFifo_dequeue(&usartIn->rxFifo, (void*)byteOut) ? CXA_IOSTREAM_READSTAT_GOTDATA : CXA_IOSTREAM_READSTAT_NODATA;
}


static bool ioStream_cb_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn)
{
	cxa_ble112_usart_t* usartIn = (cxa_ble112_usart_t*)userVarIn;
	cxa_assert(usartIn);

	for( size_t i = 0; i < bufferSize_bytesIn; i++ )
	{
		switch( usartIn->id )
		{
			case CXA_BLE112_USART_0:
				U0DBUF = ((uint8_t*)buffIn)[i];
				while (UTX0IF == 0);
				UTX0IF = 0;

				break;

			case CXA_BLE112_USART_1:
				U1DBUF = ((uint8_t*)buffIn)[i];
				while (UTX1IF == 0);
		        UTX1IF = 0;
				
				break;
		}
	}

	return true;
}
