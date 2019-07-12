/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_atmega_led_pwm8.h"


// ******** includes ********
#include <cxa_assert.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_setBrightness(cxa_led_runLoop_t *const superIn, uint8_t brightness_255In);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_atmega_led_pwm8_init(cxa_atmega_led_pwm8_t *const ledIn, cxa_atmega_timer8_ocr_t *const ocrIn, int threadIdIn)
{
	cxa_assert(ledIn);
	cxa_assert(ocrIn);

	// save our references
	ledIn->ocr = ocrIn;

	// initialize our super class (since it sets our initial value)
	cxa_led_runLoop_init(&ledIn->super, scm_setBrightness, threadIdIn);
}


// ******** local function implementations ********
static void scm_setBrightness(cxa_led_runLoop_t *const superIn, uint8_t brightness_255In)
{
	cxa_atmega_led_pwm8_t* ledIn = (cxa_atmega_led_pwm8_t*)superIn;
	cxa_assert(ledIn);

	cxa_atmega_timer8_ocr_setValue(ledIn->ocr, brightness_255In);
}
