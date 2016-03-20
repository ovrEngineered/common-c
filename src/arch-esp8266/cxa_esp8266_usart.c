/**
 * Original Source:
 * https://github.com/esp8266/Arduino.git
 *
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
 *
 * @author Christopher Armenio
 */
#include "cxa_esp8266_usart.h"


// ******** includes ********
#include <stdbool.h>
#include <cxa_delay.h>
#include <cxa_assert.h>
#include <esp/uart.h>
#include <esp/gpio.h>
#include <esp/iomux.h>
#include <espressif/esp_system.h>


// ******** local macro definitions ********
// borrowed from Hristo Gochkov (ESP8266 Arduino Project)...will translate to
// appropriate iomux calls once I figure them out...
#define ESP8266_REG(addr) 	*((volatile uint32_t *)(0x60000000+(addr)))
#define GPC(p)				ESP8266_REG(0x328 + ((p & 0xF) * 4))
#define GPEC   				ESP8266_REG(0x314) //GPIO_ENABLE_CLR WO
#define GPF(p) 				ESP8266_REG(0x800 + esp8266_gpioToFn[(p & 0xF)])
#define GPFFS(f) 			(((((f) & 4) != 0) << GPFFS2) | ((((f) & 2) != 0) << GPFFS1) | ((((f) & 1) != 0) << GPFFS0))
#define GPFFS_BUS(p) 		(((p)==1||(p)==3)?0:((p)==2||(p)==12||(p)==13||(p)==14||(p)==15)?2:((p)==0)?4:1)

#define GPFFS0 				4	//Function Select bit 0
#define GPFFS1 				5	//Function Select bit 1
#define GPFPU  				7	//Pullup
#define GPFFS2 				8	//Function Select bit 2
#define GPCI   				7	//INT_TYPE (3bits) 0:disable,1:rising,2:falling,3:change,4:low,5:high


// ******** local type definitions ********


// ******** local function prototypes ********
static cxa_ioStream_readStatus_t ioStream_cb_readByte(uint8_t *const byteOut, void *const userVarIn);
static bool ioStream_cb_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn);


// ********  local variable declarations *********
static uint8_t esp8266_gpioToFn[] = {0x34, 0x18, 0x38, 0x14, 0x3C, 0x40, 0x1C, 0x20, 0x24, 0x28, 0x2C, 0x30, 0x04, 0x08, 0x0C, 0x10};


// ******** global function implementations ********
void cxa_esp8266_usart_init_noHH(cxa_esp8266_usart_t *const usartIn, cxa_esp8266_usartId_t idIn, const uint32_t baudRate_bpsIn, uint32_t interCharDelay_msIn)
{
	cxa_assert(usartIn);
	cxa_assert( (idIn == CXA_ESP8266_USART_0) ||
				(idIn == CXA_ESP8266_USART_1) ||
				(idIn == CXA_ESP8266_USART_0_ALTPINS) );

	// save our references
	usartIn->id = idIn;
	usartIn->interCharDelay_ms = interCharDelay_msIn;
	
	// setup our pins
	switch(usartIn->id)
	{
		case CXA_ESP8266_USART_0:
		case CXA_ESP8266_USART_0_ALTPINS:
			// enable RX pin
			GPC(3) = (GPC(3) & (0xF << GPCI)); 	//SOURCE(GPIO) | DRIVER(NORMAL) | INT_TYPE(UNCHANGED) | WAKEUP_ENABLE(DISABLED)
			GPEC = (1 << 3);					//Disable

			GPF(3) = GPFFS(GPFFS_BUS(3));		//Set mode to BUS (RX0, TX0, TX1, SPI, HSPI or CLK depending in the pin)
			GPF(3) |= (1 << GPFPU);				//enable pullup on RX

			// enable TX pin
			GPC(1) = (GPC(1) & (0xF << GPCI)); 	//SOURCE(GPIO) | DRIVER(NORMAL) | INT_TYPE(UNCHANGED) | WAKEUP_ENABLE(DISABLED)
			GPEC = (1 << 1);					//Disable

			GPF(1) = GPFFS(GPFFS_BUS(1));		//Set mode to BUS (RX0, TX0, TX1, SPI, HSPI or CLK depending in the pin)
			GPF(1) |= (1 << GPFPU);				//enable pullup on RX

			// swap the pins if desired
			if( usartIn->id == CXA_ESP8266_USART_0_ALTPINS )
			{
				sdk_system_uart_swap();
				// we require the USART0 id for further configuration...since the user never sees this
				// value again, it shouldn't be a big deal to change it internally
				usartIn->id = CXA_ESP8266_USART_0;
			}
			break;

		case CXA_ESP8266_USART_1:
			// no RX pin
			// enable TX pin
			GPC(2) = (GPC(2) & (0xF << GPCI)); 	//SOURCE(GPIO) | DRIVER(NORMAL) | INT_TYPE(UNCHANGED) | WAKEUP_ENABLE(DISABLED)
			GPEC = (1 << 2);					//Disable

			GPF(2) = GPFFS(GPFFS_BUS(2));		//Set mode to BUS (RX0, TX0, TX1, SPI, HSPI or CLK depending in the pin)
			GPF(2) |= (1 << GPFPU);				//enable pullup on RX
			break;
	}

	// set the baud rate and 8N1
	uart_set_baud(usartIn->id, baudRate_bpsIn);

	// flush the usart
	uart_flush_txfifo(usartIn->id);
	uart_flush_rxfifo(usartIn->id);

	// setup our ioStream (last once everything is setup)
	cxa_ioStream_init(&usartIn->super.ioStream);
	cxa_ioStream_bind(&usartIn->super.ioStream, ioStream_cb_readByte, ioStream_cb_writeBytes, (void*)usartIn);
}


// ******** local function implementations ********
static cxa_ioStream_readStatus_t ioStream_cb_readByte(uint8_t *const byteOut, void *const userVarIn)
{
	cxa_esp8266_usart_t* usartIn = (cxa_esp8266_usart_t*)userVarIn;
	cxa_assert(usartIn);

	// USART1 has no RX pin...odd...
	if( usartIn->id == CXA_ESP8266_USART_1 ) return CXA_IOSTREAM_READSTAT_NODATA;

	// this must be USART0
	int readVal = uart_getc_nowait(usartIn->id);
	if( readVal == -1 ) return CXA_IOSTREAM_READSTAT_NODATA;

	if( byteOut != NULL ) *byteOut = readVal;
	return CXA_IOSTREAM_READSTAT_GOTDATA;
}


static bool ioStream_cb_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn)
{
	cxa_esp8266_usart_t* usartIn = (cxa_esp8266_usart_t*)userVarIn;
	cxa_assert(usartIn);
	
	for( size_t i = 0; i < bufferSize_bytesIn; i++ )
	{
		// wait for previous transmission to finish
		while( uart_putc_nowait(usartIn->id, ((char*)buffIn)[i]) == -1 );

		cxa_delay_ms(usartIn->interCharDelay_ms);
	}
	
	return true;
}
