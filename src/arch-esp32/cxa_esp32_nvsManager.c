/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_esp32_nvsManager.h"


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
void cxa_esp32_nvsManager_init(void)
{
	if( isInit ) return;

	cxa_assert(nvs_flash_init() == ESP_OK);

	cxa_assert( nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK );
	cxa_logger_init(&logger, "nvsManager");

	isInit = true;
}


bool cxa_nvsManager_doesKeyExist(const char *const keyIn)
{
	cxa_assert(isInit);

	uint8_t tmpStr;
	size_t tmpSize = sizeof(tmpStr);
	esp_err_t retVal = nvs_get_str(handle, keyIn, (char*)&tmpStr, &tmpSize);
	if( retVal != ESP_ERR_NVS_NOT_FOUND ) return true;

	uint32_t tmpUint32;
	retVal = nvs_get_u32(handle, keyIn, &tmpUint32);
	if( retVal != ESP_ERR_NVS_NOT_FOUND ) return true;

	return false;
}


bool cxa_nvsManager_get_cString(const char *const keyIn, char *const valueOut, size_t maxOutputSize_bytes)
{
	cxa_assert(isInit);

	// first, determine the size of the stored string
	esp_err_t retVal = nvs_get_str(handle, keyIn, valueOut, &maxOutputSize_bytes);
	if( retVal != ESP_OK ) cxa_logger_warn(&logger, "get error: %d", retVal);
	return (retVal == ESP_OK);
}


bool cxa_nvsManager_set_cString(const char *const keyIn, char *const valueIn)
{
	cxa_assert(isInit);

	esp_err_t retVal = nvs_set_str(handle, keyIn, valueIn);
	if( retVal != ESP_OK ) cxa_logger_warn(&logger, "set error: %d", retVal);
	return (retVal == ESP_OK);
}


bool cxa_nvsManager_get_uint8(const char *const keyIn, uint8_t *const valueOut)
{
	cxa_assert(isInit);

	esp_err_t retVal = nvs_get_u8(handle, keyIn, valueOut);
	if( retVal != ESP_OK ) cxa_logger_warn(&logger, "get error: %d", retVal);
	return (retVal == ESP_OK);
}


bool cxa_nvsManager_set_uint8(const char *const keyIn, uint8_t valueIn)
{
	cxa_assert(isInit);

	esp_err_t retVal = nvs_set_u8(handle, keyIn, valueIn);
	if( retVal != ESP_OK ) cxa_logger_warn(&logger, "set error: %d", retVal);
	return (retVal == ESP_OK);
}


bool cxa_nvsManager_get_uint32(const char *const keyIn, uint32_t *const valueOut)
{
	cxa_assert(isInit);

	esp_err_t retVal = nvs_get_u32(handle, keyIn, valueOut);
	if( retVal != ESP_OK ) cxa_logger_warn(&logger, "get error: %d %s", retVal, esp_err_to_name(retVal));
	return (retVal == ESP_OK);
}


bool cxa_nvsManager_set_uint32(const char *const keyIn, uint32_t valueIn)
{
	cxa_assert(isInit);

	esp_err_t retVal = nvs_set_u32(handle, keyIn, valueIn);
	if( retVal != ESP_OK ) cxa_logger_warn(&logger, "set error: %d", retVal);
	return (retVal == ESP_OK);
}


bool cxa_nvsManager_get_float(const char *const keyIn, float *const valueOut)
{
	return cxa_nvsManager_get_uint32(keyIn, (uint32_t*)valueOut);
}


void cxa_nvsManager_get_float_withDefault(const char *const keyIn, float *const valueOut, float defaultIn)
{
	cxa_nvsManager_get_uint32_withDefault(keyIn, (uint32_t*)valueOut, *((uint32_t*)&defaultIn));
}


bool cxa_nvsManager_set_float(const char *const keyIn, float valueIn)
{
	return cxa_nvsManager_set_uint32(keyIn, *((uint32_t*)&valueIn));
}


bool cxa_nvsManager_get_blob(const char *const keyIn, uint8_t *const valueOut, size_t maxOutputSize_bytesIn, size_t *const actualOutputSize_bytesOut)
{
	cxa_assert(isInit);

	esp_err_t retVal = nvs_get_blob(handle, keyIn, valueOut, &maxOutputSize_bytesIn);
	if( retVal != ESP_OK )
	{
		cxa_logger_warn(&logger, "get error: %d", retVal);
	}
	else
	{
		if( actualOutputSize_bytesOut != NULL ) *actualOutputSize_bytesOut = maxOutputSize_bytesIn;
	}

	return (retVal == ESP_OK);
}


bool cxa_nvsManager_set_blob(const char *const keyIn, uint8_t *const valueIn, size_t blobSize_bytesIn)
{
	cxa_assert(isInit);

	esp_err_t retVal = nvs_set_blob(handle, keyIn, valueIn, blobSize_bytesIn);
	if( retVal != ESP_OK ) cxa_logger_warn(&logger, "set error: %d", retVal);
	return (retVal == ESP_OK);
}


bool cxa_nvsManager_erase(const char *const keyIn)
{
	cxa_assert(isInit);

	esp_err_t retVal = nvs_erase_key(handle, keyIn);
	if( retVal != ESP_OK ) cxa_logger_warn(&logger, "erase error: %d", retVal);
	return (retVal == ESP_OK);
}


bool cxa_nvsManager_eraseAll(void)
{
	cxa_assert(isInit);

	esp_err_t retVal = nvs_erase_all(handle);
	if( retVal != ESP_OK ) cxa_logger_warn(&logger, "erase error: %d", retVal);
	return (retVal == ESP_OK);
}


bool cxa_nvsManager_commit(void)
{
	cxa_assert(isInit);

	esp_err_t retVal = nvs_commit(handle);
	if( retVal != ESP_OK ) cxa_logger_warn(&logger, "commit error: %d", retVal);
	return (retVal == ESP_OK);
}


// ******** local function implementations ********
