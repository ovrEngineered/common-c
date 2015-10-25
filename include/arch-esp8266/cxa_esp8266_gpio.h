/**
 * @file
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
#ifndef CXA_ESP8266_GPIO_H_
#define CXA_ESP8266_GPIO_H_


// ******** includes ********
#include <stdint.h>
#include <cxa_gpio.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_esp8266_gpio_t object
 */
typedef struct cxa_esp8266_gpio cxa_esp8266_gpio_t;


/**
 * @private
 */
struct cxa_esp8266_gpio
{
	cxa_gpio_t super;

	uint8_t pinNum;
	cxa_gpio_direction_t direction;
	cxa_gpio_polarity_t polarity;

	bool lastSetVal_nonInverted;
};


// ******** global function prototypes ********
void cxa_esp8266_gpio_init_input(cxa_esp8266_gpio_t *const gpioIn, const uint8_t pinNumIn, const cxa_gpio_polarity_t polarityIn);
void cxa_esp8266_gpio_init_output(cxa_esp8266_gpio_t *const gpioIn, const uint8_t pinNumIn, const cxa_gpio_polarity_t polarityIn, const bool initValIn);
void cxa_esp8266_gpio_init_safe(cxa_esp8266_gpio_t *const gpioIn, const uint8_t pinNumIn);


#endif // CXA_ESP8266_GPIO_H_
