/**
 * Copyright 2015 opencxa.org
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
#include "cxa_libmraa_usart.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdbool.h>
#include <cxa_assert.h>
#include <cxa_delay.h>

#include <string.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static cxa_ioStream_readStatus_t ioStream_cb_readByte(uint8_t *const byteOut, void *const userVarIn);
static bool ioStream_cb_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
bool cxa_libmraa_usart_init_noHH(cxa_libmraa_usart_t *const usartIn, int uartIndexIn, unsigned int baudRate_bpsIn)
{
	cxa_assert(usartIn);

	// setup our mraa context
	usartIn->mraaUart = mraa_uart_init(uartIndexIn);
	if( !usartIn->mraaUart ) return false;

	// set our baud rate
	mraa_uart_set_baudrate(usartIn->mraaUart, baudRate_bpsIn);
	mraa_uart_set_mode(usartIn->mraaUart, 8, MRAA_UART_PARITY_NONE, 1);
	mraa_uart_set_flowcontrol(usartIn->mraaUart, 0, 0);

	// setup our ioStream (last once everything is setup)
	cxa_ioStream_init(&usartIn->super.ioStream);
	cxa_ioStream_bind(&usartIn->super.ioStream, ioStream_cb_readByte, ioStream_cb_writeBytes, (void*)usartIn);

	return true;
}


void cxa_libmraa_usart_close(cxa_libmraa_usart_t *const usartIn)
{
	cxa_assert(usartIn);

	mraa_uart_stop(usartIn->mraaUart);
}


// ******** local function implementations ********
static cxa_ioStream_readStatus_t ioStream_cb_readByte(uint8_t *const byteOut, void *const userVarIn)
{
	cxa_libmraa_usart_t* usartIn = (cxa_libmraa_usart_t*)userVarIn;
	cxa_assert(usartIn);

	cxa_ioStream_readStatus_t retVal = CXA_IOSTREAM_READSTAT_ERROR;
	// perform our read and check the return value
	if( mraa_uart_data_available(usartIn->mraaUart, 0) )
	{
		// data available to read
		int readStat = mraa_uart_read(usartIn->mraaUart, (char*)byteOut, 1);
		retVal = (readStat < 0) ? CXA_IOSTREAM_READSTAT_ERROR : CXA_IOSTREAM_READSTAT_GOTDATA;
	}
	else
	{
		retVal = CXA_IOSTREAM_READSTAT_NODATA;
	}

	return retVal;
}


static bool ioStream_cb_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn)
{
	cxa_libmraa_usart_t* usartIn = (cxa_libmraa_usart_t*)userVarIn;
	cxa_assert(usartIn);

	size_t numBytesRemaining = bufferSize_bytesIn;
	size_t numBytesSent = 0;
	while( numBytesRemaining != 0 )
	{
		int retVal_write = mraa_uart_write(usartIn->mraaUart, (const char*)(&((uint8_t*)buffIn)[numBytesSent]), numBytesRemaining);
		if( retVal_write < 0 ) return false;

		// if we made it here, retVal_write is positive
		numBytesSent += (size_t)retVal_write;
		numBytesRemaining -= (size_t)retVal_write;

		//@todo fix this
		// add a delay otherwise bytes get dropped
		cxa_delay_ms(50);
	}

	return true;
}
