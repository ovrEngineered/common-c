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
typedef struct
{
	cxa_ble112_usart_id_t usartId;
	cxa_ble112_usart_t *cxaUsart;
}ble112Usart_cxaUsart_map_entry_t;


// ******** local function prototypes ********
static void commonInit(cxa_ble112_usart_t *const usartIn, const cxa_ble112_usart_id_t idIn, const cxa_ble112_usart_pinConfig_t pinConfigIn, const cxa_ble112_usart_baudRate_t baudRateIn, bool flowControlEnabledIn);

static void ble112CxaUsartMap_setCxaUsart(cxa_ble112_usart_id_t idIn, cxa_ble112_usart_t *const cxaUsartIn);
static cxa_ble112_usart_t* ble112CxaUsartMap_getCxaUsart_fromAvrUsart(cxa_ble112_usart_id_t idIn);
static void usart_setBaudRate(const cxa_ble112_usart_id_t portIdIn, const cxa_ble112_usart_baudRate_t baudRateIn);

static cxa_ioStream_readStatus_t ioStream_cb_readByte(uint8_t *const byteOut, void *const userVarIn);
static bool ioStream_cb_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn);

static inline void rxIsr(cxa_ble112_usart_id_t idIn);


// ********  local variable declarations *********
static ble112Usart_cxaUsart_map_entry_t ble112CxaUsartMap[] = {
	{CXA_BLE112_USART_0, NULL},
	{CXA_BLE112_USART_1, NULL},
};


// ******** global function implementations ********
void cxa_ble112_usart_init_noHH(cxa_ble112_usart_t *const usartIn, const cxa_ble112_usart_id_t idIn, const cxa_ble112_usart_pinConfig_t pinConfigIn, const cxa_ble112_usart_baudRate_t baudRateIn)
{
	commonInit(usartIn, idIn, pinConfigIn, baudRateIn, false);
}


void cxa_ble112_usart_init_HH(cxa_ble112_usart_t *const usartIn, const cxa_ble112_usart_id_t idIn, const cxa_ble112_usart_pinConfig_t pinConfigIn, const cxa_ble112_usart_baudRate_t baudRateIn)
{
	commonInit(usartIn, idIn, pinConfigIn, baudRateIn, true);
}


// ******** local function implementations ********
static void commonInit(cxa_ble112_usart_t *const usartIn, const cxa_ble112_usart_id_t idIn, const cxa_ble112_usart_pinConfig_t pinConfigIn, const cxa_ble112_usart_baudRate_t baudRateIn, bool flowControlEnabledIn)
{
	cxa_assert(usartIn);
	// save our internal state
	usartIn->id = idIn;

	// setup our receive fifo
	cxa_fixedFifo_init(&usartIn->rxFifo, CXA_FF_ON_FULL_DROP, sizeof(*usartIn->rxFifo_raw), (void*)usartIn->rxFifo_raw, sizeof(usartIn->rxFifo_raw));
	//cxa_fixedFifo_addListener(&usartIn->rxFifo, fifo_cb_noLongerFull, (void*)usartIn);

	// configure our port...
	if( (idIn == CXA_BLE112_USART_0) && (pinConfigIn == CXA_BLE112_USART_PINCONFIG_ALT1) )
	{
		PERCFG &= ~0x01;
		P0SEL |= 0x0C | (flowControlEnabledIn ? 0x30 : 0x00);
		P2DIR = (P2DIR & (~0xC0));			// priority for PORT0 to USART0
		U0CSR |= 0xC0;
		UTX0IF = 0;
		URX0IF = 0;
		U0UCR |= (flowControlEnabledIn ? 0x40 : 0x00);
	}
	else if( (idIn == CXA_BLE112_USART_0) && (pinConfigIn == CXA_BLE112_USART_PINCONFIG_ALT2) )
	{
		PERCFG |= 0x01;
		P1SEL |= 0x30 | (flowControlEnabledIn ? 0xC0 : 0x00);
		P2SEL &= 0x48;						// priority for PORT1 to USART
		U0CSR |= 0xC0;
		UTX0IF = 0;
		URX0IF = 0;
		U0UCR |= (flowControlEnabledIn ? 0x40 : 0x00);
	}
	else if( (idIn == CXA_BLE112_USART_1) && (pinConfigIn == CXA_BLE112_USART_PINCONFIG_ALT1) )
	{
		PERCFG &= ~0x02;
		P0SEL |= 0x30 | (flowControlEnabledIn ? 0x0C : 0x00);
		P2DIR = (P2DIR & (~0xC0)) | 0x40;	// priority for PORT0 to USART1
		U1CSR |= 0xC0;
		UTX1IF = 0;
		URX1IF = 0;
		U1UCR |= (flowControlEnabledIn ? 0x40 : 0x00);
	}
	else if( (idIn == CXA_BLE112_USART_1) && (pinConfigIn == CXA_BLE112_USART_PINCONFIG_ALT2) )
	{
		PERCFG |= 0x02;
		P1SEL |= 0xC0 | (flowControlEnabledIn ? 0x30 : 0x00);
		P2SEL = (P2SEL & (~0x60)) | 0x40;	// priority for PORT1 to USART1
		U1CSR |= 0xC0;
		UTX1IF = 0;
		URX1IF = 0;
		U1UCR |= (flowControlEnabledIn ? 0x40 : 0x00);
	}

	// set our baud rate
	usart_setBaudRate(idIn, baudRateIn);

	// associate this cxa usart with the ble112Usart (for interrupts)
	ble112CxaUsartMap_setCxaUsart(usartIn->id, usartIn);

	// enable reception interrupt
	URX1IE = 1;
	IEN0 |= 0x84; // 1000 0100

	// setup our ioStream (last once everything is setup)
	cxa_ioStream_init(&usartIn->super.ioStream);
	cxa_ioStream_bind(&usartIn->super.ioStream, ioStream_cb_readByte, ioStream_cb_writeBytes, (void*)usartIn);
}


static void ble112CxaUsartMap_setCxaUsart(cxa_ble112_usart_id_t idIn, cxa_ble112_usart_t *const cxaUsartIn)
{
	cxa_assert((idIn == CXA_BLE112_USART_0) || (idIn == CXA_BLE112_USART_1));
	cxa_assert(cxaUsartIn);

	// iterate through our map to find our ble112Usart id
	for( size_t i = 0; i < (sizeof(ble112CxaUsartMap)/sizeof(*ble112CxaUsartMap)); i++ )
	{
		ble112Usart_cxaUsart_map_entry_t *const currEntry = &ble112CxaUsartMap[i];
		if( currEntry->usartId == idIn )
		{
			// we have a match, save the cxaUsart
			currEntry->cxaUsart = cxaUsartIn;
			return;
		}
	}

	// if we made it here, we couldn't find a matching avrUsart
	cxa_assert(0);
}


static cxa_ble112_usart_t* ble112CxaUsartMap_getCxaUsart_fromAvrUsart(cxa_ble112_usart_id_t idIn)
{
	cxa_assert((idIn == CXA_BLE112_USART_0) || (idIn == CXA_BLE112_USART_1));

	// iterate through our map to find our avrUsart
	for( size_t i = 0; i < (sizeof(ble112CxaUsartMap)/sizeof(*ble112CxaUsartMap)); i++ )
	{
		ble112Usart_cxaUsart_map_entry_t *const currEntry = &ble112CxaUsartMap[i];
		if( currEntry->usartId == idIn ) return currEntry->cxaUsart;
	}

	// if we made it here, we couldn't find a matching avrUsart
	cxa_assert(0);
	return NULL;
}


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

	cxa_ioStream_readStatus_t retVal = cxa_fixedFifo_dequeue(&usartIn->rxFifo, (void*)byteOut) ? CXA_IOSTREAM_READSTAT_GOTDATA : CXA_IOSTREAM_READSTAT_NODATA;

	// we just dequeued a byte...give our usart a chance to handle some of the
	// fifo before re-enabling reception
	if( cxa_fixedFifo_getCurrSize(&usartIn->rxFifo) < (cxa_fixedFifo_getMaxSize(&usartIn->rxFifo)/2) )
	{
		switch( usartIn->id )
		{
			case CXA_BLE112_USART_0:
				if( !URX0IE )
				{
					URX0IE = 1;
					// force an interrupt to get the waiting byte
					if( (U0CSR & 0x04) ) URX0IF = 1;
				}
				break;

			case CXA_BLE112_USART_1:
				if( !URX1IE )
				{
					URX1IE = 1;
					// force an interrupt to get the waiting byte
					if( (U1CSR & 0x04) ) URX1IF = 1;
				}
				break;
		}
	}

	return retVal;
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


static inline void rxIsr(cxa_ble112_usart_id_t idIn)
{
	cxa_ble112_usart_t *const cxaUsartIn = ble112CxaUsartMap_getCxaUsart_fromAvrUsart(idIn);
	cxa_assert(cxaUsartIn);

	uint8_t rxChar;
	switch( cxaUsartIn->id )
	{
		case CXA_BLE112_USART_0:
			// see if we have room for the received byte
			if( cxa_fixedFifo_isFull(&cxaUsartIn->rxFifo) )
			{
				// no room...disable further interrupts (until the fifo is clear)
				URX0IE = 0;
			}
			else
			{
				// we've got room...queue the byte
				rxChar = U0DBUF;
				cxa_fixedFifo_queue(&cxaUsartIn->rxFifo, (void*)&rxChar);
			}
			break;

		case CXA_BLE112_USART_1:
			// see if we have room for the received byte
			if( cxa_fixedFifo_isFull(&cxaUsartIn->rxFifo) )
			{
				// no room...disable further interrupts (until the fifo is clear)
				URX1IE = 0;
			}
			else
			{
				// we've got room...queue the byte
				rxChar = U1DBUF;
				cxa_fixedFifo_queue(&cxaUsartIn->rxFifo, (void*)&rxChar);
			}
			break;
	}
}


#pragma vector=URX0_VECTOR
__interrupt void usart0_rx(void)
{
	rxIsr(CXA_BLE112_USART_0);
}


#pragma vector=URX1_VECTOR
__interrupt void usart1_rx(void)
{
	rxIsr(CXA_BLE112_USART_1);
}
