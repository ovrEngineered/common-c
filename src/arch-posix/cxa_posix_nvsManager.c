/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
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
static bool getKeyAndDir(const char *const keyIn, char *const keyAndDirInOut, size_t maxKeyAndDirSize_bytesIn);


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
	if( !getKeyAndDir(keyIn, keyAndDir, sizeof(keyAndDir)) ) return false;

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


bool cxa_nvsManager_get_blob(const char *const keyIn, uint8_t *const valueOut, size_t maxOutputSize_bytesIn, size_t *const actualOutputSize_bytesOut)
{
	cxa_assert(isInit);
	cxa_assert(keyIn);

	char keyAndDir[64];
	if( !getKeyAndDir(keyIn, keyAndDir, sizeof(keyAndDir)) ) return false;

	FILE* file = fopen(keyAndDir, "r");
	if( file == NULL ) return false;

	if( valueOut != NULL )
	{
		size_t numBytesRead = 0;

		// read one byte at a time
		while( fread(((void*)&valueOut[numBytesRead]), 1, 1, file) == 1 )
		{
			numBytesRead++;
			if( numBytesRead == maxOutputSize_bytesIn ) break;
		}

		// make sure we didn't have an error
		if( ferror(file) != 0 )
		{
			fclose(file);
			return false;
		}

		// set our output size if desired
		if( actualOutputSize_bytesOut != NULL ) *actualOutputSize_bytesOut = numBytesRead;
	}

	// close our file
	fclose(file);

	return true;
}


bool cxa_nvsManager_set_blob(const char *const keyIn, uint8_t *const valueIn, size_t blobSize_bytesIn)
{
	cxa_assert(isInit);
	cxa_assert(keyIn);

	char keyAndDir[64];
	if( !getKeyAndDir(keyIn, keyAndDir, sizeof(keyAndDir)) ) return false;

	FILE* file = fopen(keyAndDir, "w");
	if( file == NULL ) return false;

	if( fwrite((void*)valueIn, blobSize_bytesIn, 1, file) != 1 )
	{
		fclose(file);
		return false;
	}

	fclose(file);
	return true;
}


bool cxa_nvsManager_erase(const char *const keyIn)
{
	cxa_assert(isInit);
	cxa_assert(keyIn);

	char keyAndDir[64];
	if( !getKeyAndDir(keyIn, keyAndDir, sizeof(keyAndDir)) ) return false;

	return (remove(keyAndDir) == 0);
}


bool cxa_nvsManager_commit(void)
{
	cxa_assert(isInit);
	cxa_assert_failWithMsg("not implemented");
	return false;
}


// ******** local function implementations ********
static bool getKeyAndDir(const char *const keyIn, char *const keyAndDirInOut, size_t maxKeyAndDirSize_bytesIn)
{
	memset(keyAndDirInOut, 0, maxKeyAndDirSize_bytesIn);
	if( !cxa_stringUtils_concat(keyAndDirInOut, NVS_DIR, maxKeyAndDirSize_bytesIn) ) return false;
	if( !cxa_stringUtils_concat(keyAndDirInOut, "/", maxKeyAndDirSize_bytesIn) ) return false;
	if( !cxa_stringUtils_concat(keyAndDirInOut, keyIn, maxKeyAndDirSize_bytesIn) ) return false;

	return true;
}
