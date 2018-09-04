/**
 * @file
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
#ifndef CXA_ATMEGA_TIMER8_OCR_H_
#define CXA_ATMEGA_TIMER8_OCR_H_


// ******** includes ********
#include <stdint.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_atmega_timer8_ocr cxa_atmega_timer8_ocr_t;


/**
 * @public
 */
typedef enum
{
	CXA_ATM_TIMER8_OCR_MODE_DISCONNECTED = 0,
	CXA_ATM_TIMER8_OCR_MODE_TOGGLE = 1,
	CXA_ATM_TIMER8_OCR_MODE_NON_INVERTING = 2,
	CXA_ATM_TIMER8_OCR_MODE_INVERTING = 3
}cxa_atmega_timer8_ocr_mode_t;


/**
 * @private
 */
typedef struct cxa_atmega_timer8 cxa_atmega_timer8_t;


/**
 * @private
 */
struct cxa_atmega_timer8_ocr
{
	cxa_atmega_timer8_t* parent;
};



// ******** global function prototypes ********
void cxa_atmega_timer8_ocr_configure(cxa_atmega_timer8_ocr_t *const ocrIn, const cxa_atmega_timer8_ocr_mode_t modeIn);

void cxa_atmega_timer8_ocr_setValue(cxa_atmega_timer8_ocr_t *const ocrIn, const uint8_t valueIn);

#endif
