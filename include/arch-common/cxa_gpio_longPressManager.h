/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_GPIO_LONGPRESSMANAGER_H_
#define CXA_GPIO_LONGPRESSMANAGER_H_


// ******** includes ********
#include <cxa_array.h>
#include <cxa_config.h>
#include <cxa_gpio.h>


// ******** global macro definitions ********
#ifndef CXA_GPIO_LONGPRESSMANAGER_MAXNUM_SEGMENTS
	#define CXA_GPIO_LONGPRESSMANAGER_MAXNUM_SEGMENTS		3
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef void (*cxa_gpio_longPressManager_cb_t)(void* userVarIn);


/**
 * @private
 */
typedef struct
{
	uint16_t minHoldTime_msIn;
	uint16_t maxHoldTime_msIn;

	cxa_gpio_longPressManager_cb_t cb_segmentEnter;
	cxa_gpio_longPressManager_cb_t cb_segmentLeave;
	cxa_gpio_longPressManager_cb_t cb_segmentSelected;

	void* userVar;
}cxa_gpio_longPressManager_segment_t;


/**
 * @private
 */
typedef struct
{
	cxa_gpio_t* gpio;
	bool gpio_lastVal;

	uint32_t holdStartTime_ms;
	cxa_gpio_longPressManager_segment_t* currSegment;

	cxa_array_t segments;
	cxa_gpio_longPressManager_segment_t segments_raw[CXA_GPIO_LONGPRESSMANAGER_MAXNUM_SEGMENTS];
}cxa_gpio_longPressManager_t;


// ******** global function prototypes ********
void cxa_gpio_longPressManager_init(cxa_gpio_longPressManager_t *const lpmIn, cxa_gpio_t *const gpioIn, int threadIdIn);

bool cxa_gpio_longPressManager_addSegment(cxa_gpio_longPressManager_t *const lpmIn,
										  uint16_t minHoldTime_msIn, uint16_t maxHoldTime_msIn,
										  cxa_gpio_longPressManager_cb_t cb_segmentEnterIn,
										  cxa_gpio_longPressManager_cb_t cb_segmentLeaveIn,
										  cxa_gpio_longPressManager_cb_t cb_segmentSelectedIn,
										  void* userVarIn);

bool cxa_gpio_longPressManager_isPressed(cxa_gpio_longPressManager_t *const lpmIn);

#endif
