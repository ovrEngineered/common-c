/**
 * @file
 *
 * #### Example Usage: ####
 *
 * @code
 * @endcode
 *
 *
 * @copyright 2018 opencxa.org
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
#ifndef CXA_ATMEGA_LED_PWM8_H_
#define CXA_ATMEGA_LED_PWM8_H_


// ******** includes ********
#include <cxa_led.h>
#include <cxa_timeDiff.h>
#include <cxa_atmega_timer8_ocr.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct
{
	cxa_led_t super;

	cxa_atmega_timer8_ocr_t* ocr;

	cxa_timeDiff_t td_gp;

	struct
	{
		uint32_t onPeriod_ms;
		uint32_t offPeriod_ms;
	}blink;

	struct
	{
		uint32_t period_ms;
	}flash;
}cxa_atmega_led_pwm8_t;


// ******** global function prototypes ********
void cxa_atmega_led_pwm8_init(cxa_atmega_led_pwm8_t *const ledIn, cxa_atmega_timer8_ocr_t *const ocrIn, int threadIdIn);


#endif /* CXA_ATMEGA_LED_PWM8_H_ */
