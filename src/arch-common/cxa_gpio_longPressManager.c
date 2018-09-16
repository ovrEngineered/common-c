/**
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
#include "cxa_gpio_longPressManager.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_runLoop.h>
#include <cxa_timeBase.h>


#define CXA_LOG_LEVEL		CXA_LOG_LEVELTRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static cxa_gpio_longPressManager_segment_t* getSegmentForTime(cxa_gpio_longPressManager_t *const lpmIn, uint32_t timeIn);
static void cb_onRunLoopUpdate(void* userVarIn);
static void notifySegmentEnter(cxa_gpio_longPressManager_segment_t* segmentIn);
static void notifySegmentLeave(cxa_gpio_longPressManager_segment_t* segmentIn);
static void notifySegmentSelected(cxa_gpio_longPressManager_segment_t* segmentIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_gpio_longPressManager_init(cxa_gpio_longPressManager_t *const lpmIn, cxa_gpio_t *const gpioIn, int threadIdIn)
{
	cxa_assert(lpmIn);
	cxa_assert(gpioIn);

	// save our references
	lpmIn->gpio = gpioIn;
	lpmIn->gpio_lastVal = cxa_gpio_getValue(lpmIn->gpio);

	// initialize our segment array
	cxa_array_initStd(&lpmIn->segments, lpmIn->segments_raw);

	// register for runLoop updates
	cxa_runLoop_addEntry(threadIdIn, NULL, cb_onRunLoopUpdate, (void*)lpmIn);
}


bool cxa_gpio_longPressManager_addSegment(cxa_gpio_longPressManager_t *const lpmIn,
										  uint16_t minHoldTime_msIn, uint16_t maxHoldTime_msIn,
										  cxa_gpio_longPressManager_cb_t cb_segmentEnterIn,
										  cxa_gpio_longPressManager_cb_t cb_segmentLeaveIn,
										  cxa_gpio_longPressManager_cb_t cb_segmentSelectedIn,
										  void* userVarIn)
{
	cxa_assert(lpmIn);

	// make sure the min/max hold times make sense
	if( minHoldTime_msIn > maxHoldTime_msIn ) return false;

	// make sure this new segment doesn't overlap with an existing segment
	if( (getSegmentForTime(lpmIn, minHoldTime_msIn) != NULL) ||
		(getSegmentForTime(lpmIn, maxHoldTime_msIn) != NULL) ) return false;

	// if we made it here, this is a valid new segment
	cxa_gpio_longPressManager_segment_t newSegment =
	{
			.minHoldTime_msIn = minHoldTime_msIn,
			.maxHoldTime_msIn = maxHoldTime_msIn,
			.cb_segmentEnter = cb_segmentEnterIn,
			.cb_segmentLeave = cb_segmentLeaveIn,
			.cb_segmentSelected = cb_segmentSelectedIn,
			.userVar = userVarIn
	};
	return cxa_array_append(&lpmIn->segments, &newSegment);
}


bool cxa_gpio_longPressManager_isPressed(cxa_gpio_longPressManager_t *const lpmIn)
{
	cxa_assert(lpmIn);

	return cxa_gpio_getValue(lpmIn->gpio);
}


// ******** local function implementations ********
static cxa_gpio_longPressManager_segment_t* getSegmentForTime(cxa_gpio_longPressManager_t *const lpmIn, uint32_t timeIn)
{
	cxa_assert(lpmIn);

	cxa_array_iterate(&lpmIn->segments, currSegment, cxa_gpio_longPressManager_segment_t)
	{
		if( currSegment == NULL ) continue;

		if( (currSegment->minHoldTime_msIn <= timeIn) && (timeIn <= currSegment->maxHoldTime_msIn) ) return currSegment;
	}

	// if we made it here, the time doesn't fall within a known segment
	return NULL;
}


static void cb_onRunLoopUpdate(void* userVarIn)
{
	cxa_gpio_longPressManager_t* lpmIn = (cxa_gpio_longPressManager_t*)userVarIn;
	cxa_assert(lpmIn);

	uint8_t gpio_currVal = cxa_gpio_getValue(lpmIn->gpio);

	if( !lpmIn->gpio_lastVal && gpio_currVal )
	{
		// we _just_ got pressed...record our start time
		lpmIn->holdStartTime_ms = cxa_timeBase_getCount_us() / 1000;

		// set our initial segment if we have one that matches (may be NULL)
		lpmIn->currSegment = getSegmentForTime(lpmIn, 0);
		notifySegmentEnter(lpmIn->currSegment);
	}
	else if( lpmIn->gpio_lastVal && !gpio_currVal )
	{
		// we _just_ got released...notify our currently selected segment
		notifySegmentSelected(lpmIn->currSegment);
		lpmIn->currSegment = NULL;
	}
	else if( lpmIn->gpio_lastVal && gpio_currVal )
	{
		// we're being held
		uint32_t heldTime_ms = (cxa_timeBase_getCount_us() / 1000) - lpmIn->holdStartTime_ms;

		// see if we have a matching segment and see if it has changed
		cxa_gpio_longPressManager_segment_t* currSegment = getSegmentForTime(lpmIn, heldTime_ms);
		if( currSegment != lpmIn->currSegment )
		{
			notifySegmentLeave(lpmIn->currSegment);
			lpmIn->currSegment = currSegment;
			notifySegmentEnter(lpmIn->currSegment);
		}
	}

	lpmIn->gpio_lastVal = gpio_currVal;
}


static void notifySegmentEnter(cxa_gpio_longPressManager_segment_t* segmentIn)
{
	if( (segmentIn != NULL) &&
		(segmentIn->cb_segmentEnter != NULL) )
	{
		segmentIn->cb_segmentEnter(segmentIn->userVar);
	}
}


static void notifySegmentLeave(cxa_gpio_longPressManager_segment_t* segmentIn)
{
	if( (segmentIn != NULL) &&
		(segmentIn->cb_segmentLeave != NULL) )
	{
		segmentIn->cb_segmentLeave(segmentIn->userVar);
	}
}


static void notifySegmentSelected(cxa_gpio_longPressManager_segment_t* segmentIn)
{
	if( (segmentIn != NULL) &&
		(segmentIn->cb_segmentSelected != NULL) )
	{
		segmentIn->cb_segmentSelected(segmentIn->userVar);
	}
}
