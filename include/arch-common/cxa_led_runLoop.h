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
#ifndef CXA_LED_RUNLOOP_H_
#define CXA_LED_RUNLOOP_H_


// ******** includes ********
#include <cxa_led.h>
#include <cxa_gpio.h>
#include <cxa_timeDiff.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_led_runLoop cxa_led_runLoop_t;


/**
 * @public
 */
typedef void (*cxa_led_runLoop_scm_setBrightness_t)(cxa_led_runLoop_t *const superIn, const uint8_t brightness_255In);


/**
 * @private
 */
struct cxa_led_runLoop
{
	cxa_led_t super;

	cxa_timeDiff_t td_gp;

	struct
	{
		uint8_t onBrightness_255;
		bool wasOn;
		uint32_t onPeriod_ms;
		uint32_t offPeriod_ms;
	}blink;

	struct
	{
		uint32_t period_ms;
	}flash;

	struct
	{
		uint8_t lastBrightness_255;
	}solid;

	struct
	{
		cxa_led_runLoop_scm_setBrightness_t setBrightness;
	}scms;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_led_runLoop_init(cxa_led_runLoop_t *const ledIn,
						  cxa_led_runLoop_scm_setBrightness_t scm_setBrightnessIn,
						  int threadIdIn);


#endif /* CXA_LED_RUNLOOP_H_ */
