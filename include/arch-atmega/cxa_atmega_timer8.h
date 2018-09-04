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
#ifndef CXA_ATMEGA_TIMER8_H_
#define CXA_ATMEGA_TIMER8_H_


// ******** includes ********
#include <cxa_array.h>
#include <cxa_atmega_timer8_ocr.h>


// ******** global macro definitions ********
#ifndef CXA_ATMEGA_TIMER8_MAXNUM_LISTENERS
#define CXA_ATMEGA_TIMER8_MAXNUM_LISTENERS			1
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_atmega_timer8 cxa_atmega_timer8_t;


/**
 * @public
 */
typedef enum
{
	CXA_ATM_TIMER8_0,
	CXA_ATM_TIMER8_1,
	CXA_ATM_TIMER8_2,
}cxa_atmega_timer8_id_t;


/**
 * @public
 */
typedef enum
{
	CXA_ATM_TIMER8_MODE_FASTPWM,
}cxa_atmega_timer8_mode_t;


/**
 * @public
 */
typedef enum
{
	CXA_ATM_TIMER8_PRESCALE_STOPPED,
	CXA_ATM_TIMER8_PRESCALE_1,
	CXA_ATM_TIMER8_PRESCALE_8,
	CXA_ATM_TIMER8_PRESCALE_32,
	CXA_ATM_TIMER8_PRESCALE_64,
	CXA_ATM_TIMER8_PRESCALE_128,
	CXA_ATM_TIMER8_PRESCALE_256,
	CXA_ATM_TIMER8_PRESCALE_1024,
}cxa_atmega_timer8_prescaler_t;


/**
 * @public
 */
typedef void (*cxa_atmega_timer8_cb_onOverflow_t)(cxa_atmega_timer8_t *const timerIn, void *userVarIn);


/**
 * @private
 */
typedef struct
{
	cxa_atmega_timer8_cb_onOverflow_t cb_onOverflow;
	void* userVar;
}cxa_atmega_timer8_listenerEntry_t;


/**
 * @private
 */
struct cxa_atmega_timer8
{
	cxa_atmega_timer8_id_t id;
	cxa_atmega_timer8_prescaler_t prescaler;

	cxa_atmega_timer8_ocr_t ocrA;
	cxa_atmega_timer8_ocr_t ocrB;

	cxa_array_t listeners;
	cxa_atmega_timer8_listenerEntry_t listeners_raw[CXA_ATMEGA_TIMER8_MAXNUM_LISTENERS];
};


// ******** global function prototypes ********
void cxa_atmega_timer8_init(cxa_atmega_timer8_t *const t8In, const cxa_atmega_timer8_id_t idIn, const cxa_atmega_timer8_mode_t modeIn, const cxa_atmega_timer8_prescaler_t prescaleIn);

void cxa_atmega_timer8_addListener(cxa_atmega_timer8_t *const t8In, cxa_atmega_timer8_cb_onOverflow_t cb_onOverflowIn, void* userVarIn);

cxa_atmega_timer8_ocr_t* cxa_atmega_timer8_getOcrA(cxa_atmega_timer8_t const* t8In);
cxa_atmega_timer8_ocr_t* cxa_atmega_timer8_getOcrB(cxa_atmega_timer8_t const* t8In);

void cxa_atmega_timer8_enableInterrupt_overflow(cxa_atmega_timer8_t *const t8In);

uint32_t cxa_atmega_timer8_getOverflowPeriod_us(cxa_atmega_timer8_t *const t8In);


#endif
