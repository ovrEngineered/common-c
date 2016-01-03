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
#ifndef CXA_LED_H_
#define CXA_LED_H_


// ******** includes ********
#include <cxa_gpio.h>
#include <cxa_timeDiff.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef enum
{
	CXA_LED_STATE_OFF,
	CXA_LED_STATE_ON,
	CXA_LED_STATE_BLINK
}cxa_led_state_t;

typedef struct
{
	cxa_gpio_t* gpio;

	cxa_led_state_t currState;

	cxa_timeDiff_t td_blink;
	uint32_t onPeriod_ms;
	uint32_t offPeriod_ms;
}cxa_led_t;


// ******** global function prototypes ********
void cxa_led_init(cxa_led_t *const ledIn, cxa_gpio_t *const gpioIn);

void cxa_led_turnOn(cxa_led_t *const ledIn);
void cxa_led_turnOff(cxa_led_t *const ledIn);
void cxa_led_blink(cxa_led_t *const ledIn, uint32_t onPeriod_msIn, uint32_t offPeriod_msIn);

void cxa_led_update(cxa_led_t *const ledIn);


#endif /* CXA_LED_H_ */
