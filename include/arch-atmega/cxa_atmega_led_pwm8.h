/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_ATMEGA_LED_PWM8_H_
#define CXA_ATMEGA_LED_PWM8_H_


// ******** includes ********
#include <cxa_atmega_timer_ocr.h>
#include <cxa_led_runLoop.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct
{
	cxa_led_runLoop_t super;

	cxa_atmega_timer_ocr_t* ocr;
}cxa_atmega_led_pwm8_t;


// ******** global function prototypes ********
void cxa_atmega_led_pwm8_init(cxa_atmega_led_pwm8_t *const ledIn, cxa_atmega_timer_ocr_t *const ocrIn, int threadIdIn);


#endif /* CXA_ATMEGA_LED_PWM8_H_ */
