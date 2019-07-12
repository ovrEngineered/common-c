/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_dummy_usart.h"


// ******** includes ********
#include <stdbool.h>
#include <cxa_assert.h>
#include <cxa_delay.h>
#include <cxa_numberUtils.h>


#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static cxa_ioStream_readStatus_t ioStream_cb_readByte(uint8_t *const byteOut, void *const userVarIn);
static bool ioStream_cb_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_dummy_usart_init_noHH(cxa_dummy_usart_t *const usartIn, const uint32_t baudRate_bpsIn)
{
	cxa_assert(usartIn);

	// TODO: setup your hardware here

	// setup our ioStream (last once everything is setup)
	cxa_ioStream_init(&usartIn->super.ioStream);
	cxa_ioStream_bind(&usartIn->super.ioStream, ioStream_cb_readByte, ioStream_cb_writeBytes, (void*)usartIn);
}


// ******** local function implementations ********
static cxa_ioStream_readStatus_t ioStream_cb_readByte(uint8_t *const byteOut, void *const userVarIn)
{
	cxa_dummy_usart_t* usartIn = (cxa_dummy_usart_t*)userVarIn;
	cxa_assert(usartIn);

	// TODO: try to read a byte from the serial port
	// return CXA_IOSTREAM_READSTAT_ERROR if you had an error
	// return CXA_IOSTREAM_READSTAT_NODATA if there is no byte available
	// return CXA_IOSTREAM_READSTAT_GOTDATA if a byte was read (don't forget to store it in *byteOut)

	return CXA_IOSTREAM_READSTAT_NODATA;
}


static bool ioStream_cb_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn)
{
	cxa_dummy_usart_t* usartIn = (cxa_dummy_usart_t*)userVarIn;
	cxa_assert(usartIn);

	// TODO: try to write the bytes in the buffIn buffer to the serial port
	// return true if successful
	// return false on failure

	return true;
}
