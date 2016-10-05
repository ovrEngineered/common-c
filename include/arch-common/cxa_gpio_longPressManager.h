/**
 * @file
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
void cxa_gpio_longPressManager_init(cxa_gpio_longPressManager_t *const lpmIn, cxa_gpio_t *const gpioIn);

bool cxa_gpio_longPressManager_addSegment(cxa_gpio_longPressManager_t *const lpmIn,
										  uint16_t minHoldTime_msIn, uint16_t maxHoldTime_msIn,
										  cxa_gpio_longPressManager_cb_t cb_segmentEnterIn,
										  cxa_gpio_longPressManager_cb_t cb_segmentLeaveIn,
										  cxa_gpio_longPressManager_cb_t cb_segmentSelectedIn,
										  void* userVarIn);

bool cxa_gpio_longPressManager_isPressed(cxa_gpio_longPressManager_t *const lpmIn);

#endif
