/**
 * @file
 *
 * @note This object should work across all architecture-specific implementations
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * @endcode
 *
 *
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
#ifndef CXA_BLE112_WS2812STRING_H_
#define CXA_BLE112_WS2812STRING_H_


// ******** includes ********
#include <cxa_ws2812String.h>
#include <cxa_ble112_gpio.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct
{
	cxa_ws2812String_t super;

	cxa_ble112_gpio_t* gpio;
}cxa_ble112_ws2812String_t;


// ******** global function prototypes ********
void cxa_ble112_ws2812String_init(cxa_ble112_ws2812String_t *const ws2812In, cxa_ble112_gpio_t *const gpioIn,
								  cxa_ws2812String_pixelBuffer_t* pixelBuffersIn, size_t numPixelBuffersIn);


#endif /* CXA_BLE112_WS2812STRING_H_ */
