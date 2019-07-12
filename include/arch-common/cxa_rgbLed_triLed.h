/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_RGBLED_TRILED_H_
#define CXA_RGBLED_TRILED_H_


// ******** includes ********
#include <cxa_led.h>
#include <cxa_rgbLed_runLoop.h>


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
	cxa_rgbLed_runLoop_t super;

	cxa_led_t* led_r;
	cxa_led_t* led_g;
	cxa_led_t* led_b;
};


// ******** global function prototypes ********
void cxa_rgbLed_triLed_init(cxa_rgbLed_triLed_t *const ledIn,
							cxa_led_t *const led_rIn, cxa_led_t *const led_gIn, cxa_led_t *const led_bIn,
							int threadIdIn);


#endif
