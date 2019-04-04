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

#include <cxa_assert.h>


#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define NVS_NAMESPACE			"ovr"


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********
static cxa_logger_t logger;


// ******** global function implementations ********
void cxa_nvsManager_init(void)
{
	cxa_logger_init(&logger, "nvsManager");
}


bool cxa_nvsManager_doesKeyExist(const char *const keyIn)
{
	cxa_assert_failWithMsg("not implemented");
	return false;
}


bool cxa_nvsManager_get_cString(const char *const keyIn, char *const valueOut, size_t maxOutputSize_bytes)
{
	cxa_assert_failWithMsg("not implemented");
	return false;
}


bool cxa_nvsManager_set_cString(const char *const keyIn, char *const valueIn)
{
	cxa_assert_failWithMsg("not implemented");
	return false;
}


bool cxa_nvsManager_get_uint32(const char *const keyIn, uint32_t *const valueOut)
{
	cxa_assert_failWithMsg("not implemented");
	return false;
}


bool cxa_nvsManager_set_uint32(const char *const keyIn, uint32_t valueIn)
{
	cxa_assert_failWithMsg("not implemented");
	return false;
}


bool cxa_nvsManager_erase(const char *const keyIn)
{
	cxa_assert_failWithMsg("not implemented");
	return false;
}


bool cxa_nvsManager_commit(void)
{
	cxa_assert_failWithMsg("not implemented");
	return false;
}


// ******** local function implementations ********

