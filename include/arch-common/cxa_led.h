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
/**
 * @public
 */
typedef struct cxa_led cxa_led_t;


/**
 * @public
 */
typedef enum
{
	CXA_LED_STATE_OFF,
	CXA_LED_STATE_ON,
	CXA_LED_STATE_BLINK,
	CXA_LED_STATE_FLASH_ONCE
}cxa_led_state_t;


/**
 * @protected
 */
typedef void (*cxa_led_scm_turnOn_t)(cxa_led_t *const superIn);

/**
 * @protected
 */
typedef void (*cxa_led_scm_turnOff_t)(cxa_led_t *const superIn);

/**
 * @protected
 */
typedef void (*cxa_led_scm_blink_t)(cxa_led_t *const superIn, uint32_t onPeriod_msIn, uint32_t offPeriod_msIn);

/**
 * @protected
 */
typedef void (*cxa_led_scm_setBrightness_t)(cxa_led_t *const superIn, uint8_t brightnessIn);

/**
 * @protected
 */
typedef void (*cxa_led_scm_flashOnce_t)(cxa_led_t *const superIn, bool flashStateIn, uint32_t period_msIn);


struct cxa_led
{
	cxa_led_state_t currState;
	cxa_led_state_t prevState;

	cxa_led_scm_turnOn_t scm_turnOn;
	cxa_led_scm_turnOff_t scm_turnOff;
	cxa_led_scm_blink_t scm_blink;
	cxa_led_scm_setBrightness_t scm_setBrightness;
	cxa_led_scm_flashOnce_t scm_flashOnce;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_led_init(cxa_led_t *const ledIn,
				  cxa_led_scm_turnOn_t scm_turnOnIn,
				  cxa_led_scm_turnOff_t scm_turnOffIn,
				  cxa_led_scm_blink_t scm_blinkIn,
				  cxa_led_scm_setBrightness_t scm_setBrightnessIn,
				  cxa_led_scm_flashOnce_t scm_flashOnceIn);

void cxa_led_turnOn(cxa_led_t *const ledIn);
void cxa_led_turnOff(cxa_led_t *const ledIn);

void cxa_led_blink(cxa_led_t *const ledIn, uint32_t onPeriod_msIn, uint32_t offPeriod_msIn);
void cxa_led_flashOnce(cxa_led_t *const ledIn, bool flashStateIn, uint32_t period_msIn);
void cxa_led_setBrightness(cxa_led_t *const ledIn, uint8_t brightnessIn);


#endif /* CXA_LED_H_ */
