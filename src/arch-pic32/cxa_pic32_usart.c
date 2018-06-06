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
#include "cxa_pic32_usart.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdbool.h>
#include <cxa_assert.h>
#include <cxa_delay.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static cxa_ioStream_readStatus_t ioStream_cb_readByte(uint8_t *const byteOut, void *const userVarIn);
static bool ioStream_cb_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_pic32_usart_init(cxa_pic32_usart_t *const usartIn, int uartHarmonyInstanceIn)
{
	cxa_assert(usartIn);

    
	// setup our UART hardware
    usartIn->usartHandle = DRV_USART_Open(uartHarmonyInstanceIn, DRV_IO_INTENT_READWRITE|DRV_IO_INTENT_NONBLOCKING);		


	// setup our ioStream (last once everything is setup)
	cxa_ioStream_init(&usartIn->super.ioStream);
	cxa_ioStream_bind(&usartIn->super.ioStream, ioStream_cb_readByte, ioStream_cb_writeBytes, (void*)usartIn);
}


// ******** local function implementations ********
static cxa_ioStream_readStatus_t ioStream_cb_readByte(uint8_t *const byteOut, void *const userVarIn)
{
	cxa_pic32_usart_t* usartIn = (cxa_pic32_usart_t*)userVarIn;
	cxa_assert(usartIn);
	
    // make sure we don't have an error on the UART
    if( DRV_USART_ErrorGet(usartIn->usartHandle) != DRV_USART_ERROR_NONE ) return CXA_IOSTREAM_READSTAT_ERROR;
    
    //Check to see if we have a byte to receive per DRV_USART.h _ReadByte function
    if(DRV_USART_ReceiverBufferIsEmpty(usartIn->usartHandle)) return CXA_IOSTREAM_READSTAT_NODATA;
    
	uint8_t tmpBuff = DRV_USART_ReadByte(usartIn->usartHandle);
    if( byteOut != NULL ) *byteOut = tmpBuff;
    
    return CXA_IOSTREAM_READSTAT_GOTDATA;
}


static bool ioStream_cb_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn)
{
	cxa_pic32_usart_t* usartIn = (cxa_pic32_usart_t*)userVarIn;
	cxa_assert(usartIn);

    for(size_t i=0; i < bufferSize_bytesIn; i++)
    {
        while(DRV_USART_TransmitBufferIsFull(usartIn->usartHandle));
        DRV_USART_WriteByte(usartIn->usartHandle, ((uint8_t*)buffIn)[i]);
    }

	return true;
}
