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
#ifndef CXA_RGBLED_H_
#define CXA_RGBLED_H_


// ******** includes ********
#include <stdint.h>


// ******** global macro definitions ********
#define CXA_RGBLED_RED				0xFF, 0x00, 0x00
#define CXA_RGBLED_GREEN			0x00, 0xFF, 0x00
#define CXA_RGBLED_BLUE				0x00, 0x00, 0xFF
#define CXA_RGBLED_ORANGE			0xFF, 0x80, 0x00
#define CXA_RGBLED_CYAN				0x00, 0xFF, 0xFF



// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_rgbLed cxa_rgbLed_t;


/**
 * @public
 */
typedef enum
{
	CXA_RGBLED_STATE_SOLID,
	CXA_RGBLED_STATE_BLINK,
	CXA_RGBLED_STATE_FLASHONCE
}cxa_rgbLed_state_t;


/**
 * @protected
 */
typedef void (*cxa_rgbLed_scm_setRgb_t)(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn);


/**
 * @protected
 */
typedef void (*cxa_rgbLed_scm_blink_t)(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn,
									   uint16_t onPeriod_msIn, uint16_t offPeriod_msIn);

/**
 * @protected
 */
typedef void (*cxa_rgbLed_scm_flashOnce_t)(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn,
									   	   uint16_t period_msIn);

/**
 * @private
 */
struct cxa_rgbLed
{
	cxa_rgbLed_scm_setRgb_t scm_setRgb;
	cxa_rgbLed_scm_blink_t scm_blink;
	cxa_rgbLed_scm_flashOnce_t scm_flashOnce;

	cxa_rgbLed_state_t prevState;
	cxa_rgbLed_state_t currState;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_rgbLed_init(cxa_rgbLed_t *const ledIn,
					 cxa_rgbLed_scm_setRgb_t scm_setRgbIn,
					 cxa_rgbLed_scm_blink_t scm_blinkIn,
					 cxa_rgbLed_scm_flashOnce_t scm_flashOnceIn);


/**
 * @public
 */
void cxa_rgbLed_setRgb(cxa_rgbLed_t *const ledIn, uint8_t rIn, uint8_t gIn, uint8_t bIn);


/**
 * @public
 */
void cxa_rgbLed_blink(cxa_rgbLed_t *const ledIn, uint8_t rIn, uint8_t gIn, uint8_t bIn,
					  uint16_t onPeriod_msIn, uint16_t offPeriod_msIn);


/**
 * @public
 */
void cxa_rgbLed_flashOnce(cxa_rgbLed_t *const ledIn, uint8_t rIn, uint8_t gIn, uint8_t bIn,
						  uint16_t period_msIn);

#endif /* CXA_RGBLED_H_ */
