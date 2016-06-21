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


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_rgbLed cxa_rgbLed_t;


/**
 * @protected
 */
typedef void (*cxa_rgbLed_scm_setRgb_t)(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn);


struct cxa_rgbLed
{
	cxa_rgbLed_scm_setRgb_t scm_setRgb;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_rgbLed_init(cxa_rgbLed_t *const ledIn, cxa_rgbLed_scm_setRgb_t scm_setRgbIn);

void cxa_rgbLed_setRgb(cxa_rgbLed_t *const ledIn, uint8_t rIn, uint8_t gIn, uint8_t bIn);


#endif /* CXA_RGBLED_H_ */
