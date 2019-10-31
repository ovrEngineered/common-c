/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_criticalSection.h"


// ******** includes ********
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <stdbool.h>


// ******** local macro definitions ********


// ******** local type definitions ********



// ******** local function prototypes ********


// ********  local variable declarations *********
static portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;


// ******** global function implementations ********
void cxa_criticalSection_enter(void)
{
	portENTER_CRITICAL(&myMutex);
}


void cxa_criticalSection_exit(void)
{
	portEXIT_CRITICAL(&myMutex);
}


// ******** local function implementations ********
