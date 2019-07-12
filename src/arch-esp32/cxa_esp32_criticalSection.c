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
#include <freertos/semphr.h>

#include <stdbool.h>


// ******** local macro definitions ********


// ******** local type definitions ********



// ******** local function prototypes ********
static void init(void);


// ********  local variable declarations *********
static bool isInit = false;

static SemaphoreHandle_t xSemaphore;


// ******** global function implementations ********
void cxa_criticalSection_enter(void)
{
	if( !isInit ) init();

	xSemaphoreTake(xSemaphore, portMAX_DELAY);
}


void cxa_criticalSection_exit(void)
{
	if( !isInit ) init();

	xSemaphoreGive(xSemaphore);
}


// ******** local function implementations ********
static void init(void)
{
	xSemaphore = xSemaphoreCreateMutex();

	isInit = true;
}
