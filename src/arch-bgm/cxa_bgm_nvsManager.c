/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include <cxa_bgm_nvsManager.h>


// ******** includes ********
#include <stdio.h>


#include <cxa_assert.h>
#include <cxa_stringUtils.h>

#include "bg_types.h"
#include "native_gecko.h"


#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static bool getHandleForKey(const char *const keyIn, uint16_t* handleOut);


// ********  local variable declarations *********
static bool isInit = false;

static cxa_bgm_nvsManager_keyToHandleMapEntry_t* map;
static size_t numMapEntries;

static cxa_logger_t logger;


// ******** global function implementations ********
void cxa_bgm_nvsManager_init(const cxa_bgm_nvsManager_keyToHandleMapEntry_t *const mapIn, size_t numEntriesIn)
{
	if( isInit ) return;

	// save our references
	map = (cxa_bgm_nvsManager_keyToHandleMapEntry_t*)mapIn;
	numMapEntries = numEntriesIn;

	cxa_logger_init(&logger, "nvsManager");

	isInit = true;
}

bool cxa_nvsManager_doesKeyExist(const char *const keyIn)
{
	cxa_assert(isInit);

	cxa_assert_failWithMsg("not yet implemented");

	return false;
}


bool cxa_nvsManager_get_cString(const char *const keyIn, char *const valueOut, size_t maxOutputSize_bytes)
{
	cxa_assert(isInit);

	cxa_assert_failWithMsg("not yet implemented");

	return false;
}


bool cxa_nvsManager_set_cString(const char *const keyIn, char *const valueIn)
{
	cxa_assert(isInit);

	cxa_assert_failWithMsg("not yet implemented");

	return false;
}


bool cxa_nvsManager_get_uint8(const char *const keyIn, uint8_t *const valueOut)
{
	cxa_assert(isInit);

	uint16_t handle;
	cxa_assert(getHandleForKey(keyIn, &handle));

	struct gecko_msg_flash_ps_load_rsp_t* resp = gecko_cmd_flash_ps_load(handle);
	if( (resp->result == 0) && (resp->value.len == 1) )
	{
		if( valueOut != NULL ) memcpy(valueOut, resp->value.data, resp->value.len);
	}
	else
	{
		cxa_logger_warn(&logger, "get error: %d", resp->result);
	}

	return (resp->result == 0);
}


bool cxa_nvsManager_set_uint8(const char *const keyIn, uint8_t valueIn)
{
	cxa_assert(isInit);

	uint16_t handle;
	cxa_assert(getHandleForKey(keyIn, &handle));

	struct gecko_msg_flash_ps_save_rsp_t* resp = gecko_cmd_flash_ps_save(handle, 1, &valueIn);
	if( resp->result != 0 )
	{
		cxa_logger_warn(&logger, "set error: %d", resp->result);
	}

	return (resp->result == 0);
}


bool cxa_nvsManager_get_uint32(const char *const keyIn, uint32_t *const valueOut)
{
	cxa_assert(isInit);

	cxa_assert_failWithMsg("not yet implemented");

	return false;
}


bool cxa_nvsManager_set_uint32(const char *const keyIn, uint32_t valueIn)
{
	cxa_assert(isInit);

	cxa_assert_failWithMsg("not yet implemented");

	return false;
}


bool cxa_nvsManager_get_blob(const char *const keyIn, uint8_t *const valueOut, size_t maxOutputSize_bytesIn, size_t *const actualOutputSize_bytesOut)
{
	cxa_assert(isInit);

	uint16_t handle;
	cxa_assert(getHandleForKey(keyIn, &handle));

	struct gecko_msg_flash_ps_load_rsp_t* resp = gecko_cmd_flash_ps_load(handle);
	if( (resp->result == 0) && (resp->value.len <= maxOutputSize_bytesIn) )
	{
		if( valueOut != NULL ) memcpy(valueOut, resp->value.data, resp->value.len);
		if( actualOutputSize_bytesOut != NULL ) *actualOutputSize_bytesOut = resp->value.len;
	}
	else
	{
		cxa_logger_warn(&logger, "get error: %d", resp->result);
	}

	return (resp->result == 0);
}


bool cxa_nvsManager_set_blob(const char *const keyIn, uint8_t *const valueIn, size_t blobSize_bytesIn)
{
	cxa_assert(isInit);

	uint16_t handle;
	cxa_assert(getHandleForKey(keyIn, &handle));

	struct gecko_msg_flash_ps_save_rsp_t* resp = gecko_cmd_flash_ps_save(handle, blobSize_bytesIn, valueIn);
	if( resp->result != 0 )
	{
		cxa_logger_warn(&logger, "set error: %d", resp->result);
	}

	return (resp->result == 0);
}


bool cxa_nvsManager_erase(const char *const keyIn)
{
	cxa_assert(isInit);

	uint16_t handle;
	cxa_assert(getHandleForKey(keyIn, &handle));

	struct gecko_msg_flash_ps_erase_rsp_t* resp = gecko_cmd_flash_ps_erase(handle);

	return (resp->result == 0);
}


bool cxa_nvsManager_eraseAll(void)
{
	cxa_assert(isInit);

	cxa_assert_failWithMsg("not yet implemented");

	return false;
}


bool cxa_nvsManager_commit(void)
{
	cxa_assert(isInit);

	cxa_assert_failWithMsg("not yet implemented");

	return false;
}


// ******** local function implementations ********
static bool getHandleForKey(const char *const keyIn, uint16_t* handleOut)
{
	cxa_assert(keyIn);
	cxa_assert((0 < strlen(keyIn)) && (strlen(keyIn) <= CXA_BGM_NVSMANAGER_MAX_KEY_LEN_BYTES));

	for( size_t i = 0; i < numMapEntries; i++ )
	{
		cxa_bgm_nvsManager_keyToHandleMapEntry_t* currEntry = &map[i];
		if( cxa_stringUtils_equals(keyIn, currEntry->key) )
		{
			if( handleOut != NULL ) *handleOut = currEntry->handle;
			return true;
		}
	}

	cxa_logger_warn(&logger, "unknown key: '%s'", keyIn);
	return false;
}
