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
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <cxa_assert.h>
#include <cxa_stringUtils.h>


#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********
static char NVS_DIR[255];

static bool isInit = false;
static cxa_logger_t logger;


// ******** global function implementations ********
void cxa_posix_nvsManager_init(char *const nvsDirIn)
{
	cxa_assert_msg(strlen(nvsDirIn) < (sizeof(NVS_DIR)-1), "nvs directory too long");

	cxa_logger_init(&logger, "nvsManager");

	cxa_stringUtils_copy(NVS_DIR, nvsDirIn, sizeof(NVS_DIR));

	isInit = true;
}


bool cxa_nvsManager_doesKeyExist(const char *const keyIn)
{
	cxa_assert(isInit);
	cxa_assert_failWithMsg("not implemented");
	return false;
}


bool cxa_nvsManager_get_cString(const char *const keyIn, char *const valueOut, size_t maxOutputSize_bytes)
{
	cxa_assert(isInit);
	cxa_assert(keyIn);

	char keyAndDir[64];
	memset(keyAndDir, 0, sizeof(keyAndDir));
	cxa_stringUtils_concat(keyAndDir, NVS_DIR, sizeof(keyAndDir));
	cxa_stringUtils_concat(keyAndDir, "/", sizeof(keyAndDir));
	cxa_stringUtils_concat(keyAndDir, keyIn, sizeof(keyAndDir));

	FILE* file = fopen(keyAndDir, "r");
	if( file == NULL ) return false;

	if( valueOut != NULL )
	{
		size_t numBytesRead = 0;

		// read one byte at a time
		while( fread(((void*)&valueOut[numBytesRead]), 1, 1, file) == 1 )
		{
			numBytesRead++;
			if( numBytesRead == maxOutputSize_bytes ) break;
		}

		// make sure we didn't have an error
		if( ferror(file) != 0 )
		{
			fclose(file);
			return false;
		}

		// make sure everything is null terminated
		valueOut[numBytesRead] = 0;
	}

	// close our file
	fclose(file);

	return true;
}


bool cxa_nvsManager_set_cString(const char *const keyIn, char *const valueIn)
{
	cxa_assert(isInit);
	cxa_assert_failWithMsg("not implemented");
	return false;
}


bool cxa_nvsManager_get_uint32(const char *const keyIn, uint32_t *const valueOut)
{
	cxa_assert(isInit);
	cxa_assert_failWithMsg("not implemented");
	return false;
}


bool cxa_nvsManager_set_uint32(const char *const keyIn, uint32_t valueIn)
{
	cxa_assert(isInit);
	cxa_assert_failWithMsg("not implemented");
	return false;
}


bool cxa_nvsManager_erase(const char *const keyIn)
{
	cxa_assert(isInit);
	cxa_assert_failWithMsg("not implemented");
	return false;
}


bool cxa_nvsManager_commit(void)
{
	cxa_assert(isInit);
	cxa_assert_failWithMsg("not implemented");
	return false;
}


// ******** local function implementations ********

