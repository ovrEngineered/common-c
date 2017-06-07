/**
 * Copyright 2017 opencxa.org
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
 *
 * @author Christopher Armenio
 */
#include "cxa_nvsManager.h"


// ******** includes ********
#include <stdio.h>

#include "nvs.h"
#include "nvs_flash.h"

#include <cxa_assert.h>


#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define NVS_NAMESPACE			"ovr"


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********
static bool isInit = false;
static nvs_handle handle;

static cxa_logger_t logger;


// ******** global function implementations ********
void cxa_nvsManager_init(void)
{
	nvs_flash_init();

	cxa_assert( nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK );

	cxa_logger_init(&logger, "nvsManager");

	isInit = true;
}


bool cxa_nvsManager_get_cString(const char *const keyIn, char *const valueOut, size_t maxOutputSize_bytes)
{
	if( !isInit ) cxa_nvsManager_init();

	// first, determine the size of the stored string
	esp_err_t retVal = nvs_get_str(handle, keyIn, valueOut, &maxOutputSize_bytes);
	if( retVal != ESP_OK ) cxa_logger_warn(&logger, "get error: %d", retVal);
	return (retVal == ESP_OK);
}


bool cxa_nvsManager_set_cString(const char *const keyIn, char *const valueIn)
{
	if( !isInit ) cxa_nvsManager_init();

	esp_err_t retVal = nvs_set_str(handle, keyIn, valueIn);
	if( retVal != ESP_OK ) cxa_logger_warn(&logger, "set error: %d", retVal);
	return (retVal == ESP_OK);
}


bool cxa_nvsManager_erase(const char *const keyIn)
{
	if( !isInit ) cxa_nvsManager_init();

	esp_err_t retVal = nvs_erase_key(handle, keyIn);
	if( retVal != ESP_OK ) cxa_logger_warn(&logger, "erase error: %d", retVal);
	return (retVal == ESP_OK);
}


bool cxa_nvsManager_commit(void)
{
	esp_err_t retVal = nvs_commit(handle);
	if( retVal != ESP_OK ) cxa_logger_warn(&logger, "commit error: %d", retVal);
	return (retVal == ESP_OK);
}


// ******** local function implementations ********

