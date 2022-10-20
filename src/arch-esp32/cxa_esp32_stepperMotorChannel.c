/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_stepperMotorChannel.h"


// ******** includes ********
#include <cxa_assert.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/rtc.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void periodic_timer_callback(void* arg);


// ********  local variable declarations *********
static bool isTimerRunning = false;
static esp_timer_handle_t periodic_timer;

static cxa_stepperMotorChannel_timedCallback_t cb = NULL;


// ******** global function implementations ********
void cxa_stepperMotorChannel_registerTimedCallback(cxa_stepperMotorChannel_timedCallback_t cbIn, uint32_t stepFreq_hzIn)
{
	cxa_assert(cbIn);

	// save our callback
	cb = cbIn;

	// stop our timer (if we're already running)
	if( isTimerRunning )
	{
		esp_timer_stop(periodic_timer);
	}
	else
	{
		// create a new timer
		const esp_timer_create_args_t periodic_timer_args = {
					.callback = &periodic_timer_callback,
					.name = "stepCtrl"
			};
		cxa_assert(esp_timer_create(&periodic_timer_args, &periodic_timer) == ESP_OK);
	}

	// start the timer
	cxa_assert(esp_timer_start_periodic(periodic_timer, (1.0 / ((float)stepFreq_hzIn)) * 1.0E6 ) == ESP_OK);
	isTimerRunning = true;
}


// ******** local function implementations ********


// ******** interrupt implementations ********
static void periodic_timer_callback(void* arg)
{
	if( cb != NULL ) cb();
}
