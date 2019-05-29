/**
 * Copyright 2016 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

