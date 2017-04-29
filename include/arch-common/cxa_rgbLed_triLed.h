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
 * @copyright 2016 opencxa.org
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
#ifndef CXA_RGBLED_TRILED_H_
#define CXA_RGBLED_TRILED_H_


// ******** includes ********
#include <cxa_led.h>
#include <cxa_rgbLed.h>
#include <cxa_timeDiff.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_rgbLed_triLed cxa_rgbLed_triLed_t;


/**
 * @private
 */
struct cxa_rgbLed_triLed
{
	cxa_rgbLed_t super;

	cxa_led_t* led_r;
	cxa_led_t* led_g;
	cxa_led_t* led_b;

	cxa_timeDiff_t td_gp;

	struct
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
	}lastSet;

	struct
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
		bool isOn;

		uint32_t onPeriod_ms;
		uint32_t offPeriod_ms;
	}blink;

	struct
	{
		uint32_t period_ms;
	}flash;
};


// ******** global function prototypes ********
void cxa_rgbLed_triLed_init(cxa_rgbLed_triLed_t *const ledIn,
							cxa_led_t *const led_rIn, cxa_led_t *const led_gIn, cxa_led_t *const led_bIn,
							int threadIdIn);


#endif /* CXA_RGBLED_TRILED_H_ */
