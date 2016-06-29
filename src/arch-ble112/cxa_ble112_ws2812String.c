/**
 * @copyright 2015 opencxa.org
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
#include "cxa_ble112_ws2812String.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_delay.h>


// ******** local macro definitions ********
#define writeBit(bitIn, regIn, gpioBitNumIn)				\
	if( bitIn )												\
	{														\
		regIn |= (1 << gpioBitNumIn);						\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		regIn &= ~(1 << gpioBitNumIn);						\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
	}														\
	else													\
	{														\
		regIn |= (1 << gpioBitNumIn);						\
		asm("nop");											\
		regIn &= ~(1 << gpioBitNumIn);						\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
		asm("nop");											\
	}


#define writeByte(byteIn, regIn, gpioBitNumIn)				\
		writeBit(byteIn & (1 << 7), regIn, gpioBitNumIn);	\
		writeBit(byteIn & (1 << 6), regIn, gpioBitNumIn);	\
		writeBit(byteIn & (1 << 5), regIn, gpioBitNumIn);	\
		writeBit(byteIn & (1 << 4), regIn, gpioBitNumIn);	\
		writeBit(byteIn & (1 << 3), regIn, gpioBitNumIn);	\
		writeBit(byteIn & (1 << 2), regIn, gpioBitNumIn);	\
		writeBit(byteIn & (1 << 1), regIn, gpioBitNumIn);	\
		writeBit(byteIn & (1 << 0), regIn, gpioBitNumIn);


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_writeBytes(cxa_ws2812String_t *const superIn, cxa_ws2812String_pixelBuffer_t* pixelBuffersIn, size_t numPixelBuffersIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_ble112_ws2812String_init(cxa_ble112_ws2812String_t *const ws2812In, cxa_ble112_gpio_t *const gpioIn,
								  cxa_ws2812String_pixelBuffer_t* pixelBuffersIn, size_t numPixelBuffersIn)
{
	cxa_assert(ws2812In);
	cxa_assert(gpioIn);
	cxa_assert(pixelBuffersIn);
	cxa_assert(numPixelBuffersIn > 0);

	// save our references
	ws2812In->gpio = gpioIn;

	// initialize our super class (turns off all leds)
	cxa_ws2812String_init(&ws2812In->super, pixelBuffersIn, numPixelBuffersIn, scm_writeBytes);
}


// ******** local function implementations ********
static void scm_writeBytes(cxa_ws2812String_t *const superIn, cxa_ws2812String_pixelBuffer_t* pixelBuffersIn, size_t numPixelBuffersIn)
{
	cxa_ble112_ws2812String_t* ws2812In = (cxa_ble112_ws2812String_t*)superIn;
	cxa_assert(ws2812In);

	// do the actual writing
	cxa_delay_ms(50);

	// can't get pointer to __sfr, so put this in a big switch
	uint8_t pinNum = ws2812In->gpio->pinNum;
	switch( ws2812In->gpio->port )
	{
		case CXA_BLE112_GPIO_PORT_0:
			for( int i_pixBuff = 0; i_pixBuff < numPixelBuffersIn; i_pixBuff++ )
			{
				// g, r, b
				writeByte(pixelBuffersIn[i_pixBuff].g, P0, pinNum);
				writeByte(pixelBuffersIn[i_pixBuff].r, P0, pinNum);
				writeByte(pixelBuffersIn[i_pixBuff].b, P0, pinNum);
			}
			break;

		case CXA_BLE112_GPIO_PORT_1:
			for( int i_pixBuff = 0; i_pixBuff < numPixelBuffersIn; i_pixBuff++ )
			{
				// g, r, b
				writeByte(pixelBuffersIn[i_pixBuff].g, P1, pinNum);
				writeByte(pixelBuffersIn[i_pixBuff].r, P1, pinNum);
				writeByte(pixelBuffersIn[i_pixBuff].b, P1, pinNum);
			}
			break;

		case CXA_BLE112_GPIO_PORT_2:
			for( int i_pixBuff = 0; i_pixBuff < numPixelBuffersIn; i_pixBuff++ )
			{
				// g, r, b
				writeByte(pixelBuffersIn[i_pixBuff].g, P2, pinNum);
				writeByte(pixelBuffersIn[i_pixBuff].r, P2, pinNum);
				writeByte(pixelBuffersIn[i_pixBuff].b, P2, pinNum);
			}
			break;
	}

	// hold low to latch
	for( uint8_t i = 0; i < 32; i++ ) asm("nop");
}
