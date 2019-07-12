/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
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
	CXA_LED_STATE_SOLID,
	CXA_LED_STATE_BLINK,
	CXA_LED_STATE_FLASH_ONCE
}cxa_led_state_t;


/**
 * @protected
 */
typedef void (*cxa_led_scm_blink_t)(cxa_led_t *const superIn, uint8_t onBrightness_255In, uint32_t onPeriod_msIn, uint32_t offPeriod_msIn);


/**
 * @protected
 */
typedef void (*cxa_led_scm_setSolid_t)(cxa_led_t *const superIn, uint8_t brightness_255In);


/**
 * @protected
 */
typedef void (*cxa_led_scm_flashOnce_t)(cxa_led_t *const superIn, uint8_t brightness_255In, uint32_t period_msIn);


struct cxa_led
{
	cxa_led_state_t currState;
	cxa_led_state_t prevState;

	cxa_led_scm_blink_t scm_blink;
	cxa_led_scm_setSolid_t scm_setSolid;
	cxa_led_scm_flashOnce_t scm_flashOnce;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_led_init(cxa_led_t *const ledIn,
				  cxa_led_scm_setSolid_t scm_setSolidIn,
				  cxa_led_scm_blink_t scm_blinkIn,
				  cxa_led_scm_flashOnce_t scm_flashOnceIn);

void cxa_led_blink(cxa_led_t *const ledIn, uint8_t onBrightness_255In, uint32_t onPeriod_msIn, uint32_t offPeriod_msIn);
void cxa_led_flashOnce(cxa_led_t *const ledIn, uint8_t brightness_255In, uint32_t period_msIn);
void cxa_led_setSolid(cxa_led_t *const ledIn, uint8_t brightness_255In);


#endif /* CXA_LED_H_ */
