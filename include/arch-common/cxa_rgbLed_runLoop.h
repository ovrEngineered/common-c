/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_RGBLED_RUNLOOP_H_
#define CXA_RGBLED_RUNLOOP_H_


// ******** includes ********
#include <cxa_led.h>
#include <cxa_rgbLed.h>
#include <cxa_timeDiff.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_rgbLed_runLoop cxa_rgbLed_runLoop_t;


/**
 * @public
 */
typedef void (*cxa_rgbLed_runLoop_scm_setRgb_t)(cxa_rgbLed_runLoop_t *const superIn, const uint8_t r_255In, const uint8_t g_255In, const uint8_t b_255In);


/**
 * @private
 */
typedef struct
{
	uint8_t rOn_255;
	uint8_t gOn_255;
	uint8_t bOn_255;

	uint32_t period_ms;
}cxa_rgbLed_runLoop_altColorEntry_t;


/**
 * @private
 */
struct cxa_rgbLed_runLoop
{
	cxa_rgbLed_t super;

	cxa_timeDiff_t td_gp;

	struct
	{
		cxa_rgbLed_runLoop_altColorEntry_t colors[2];
		uint8_t lastColorIndex;
	}alternate;

	struct
	{
		uint32_t period_ms;
	}flash;

	struct
	{
		uint8_t lastBrightnessR_255;
		uint8_t lastBrightnessG_255;
		uint8_t lastBrightnessB_255;
	}solid;

	struct
	{
		cxa_rgbLed_runLoop_scm_setRgb_t setRgb;
	}scms;
};


// ******** global function prototypes ********
void cxa_rgbLed_runLoop_init(cxa_rgbLed_runLoop_t *const ledIn,
							 cxa_rgbLed_runLoop_scm_setRgb_t scm_setRgbIn,
							 int threadIdIn);


#endif /* CXA_RGBLED_RUNLOOP_H_ */
