/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_nvsManager.h"


// ******** includes ********
#include <stdio.h>

//ESP32
//#include "nvs.h"
//#include "nvs_flash.h"

//TIC2K
#include <AT25M02.h>
#include <cxa_assert.h>
#include <Externals.h>
#include <cxa_stringUtils.h>


#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define NVS_NAMESPACE			"ovr"


// ******** local type definitions ********


// ******** local function prototypes ********
static void init(void);
//extern void AT25LoadMAC(void);

// ********  local variable declarations *********
//static bool isInit = false;
////static nvs_handle handle;
//
//static cxa_logger_t logger;


// ******** global function implementations ********
//bool cxa_esp32_nvsManager_init(void)
//{
//	return (nvs_flash_init() == ESP_OK);
//}


//bool cxa_nvsManager_doesKeyExist(const char *const keyIn)
//{
//	if( !isInit ) init();
//
//	uint8_t tmpStr;
//	size_t tmpSize = sizeof(tmpStr);
//	esp_err_t retVal = nvs_get_str(handle, keyIn, (char*)&tmpStr, &tmpSize);
//	if( retVal != ESP_ERR_NVS_NOT_FOUND ) return true;
//
//	uint32_t tmpUint32;
//	retVal = nvs_get_u32(handle, keyIn, &tmpUint32);
//	if( retVal != ESP_ERR_NVS_NOT_FOUND ) return true;
//
//	return false;
//}


//bool cxa_nvsManager_get_cString(const char *const keyIn, char *const valueOut, size_t maxOutputSize_bytes)
//{
//	if( !isInit ) init();
//
//	// first, determine the size of the stored string
//	esp_err_t retVal = nvs_get_str(handle, keyIn, valueOut, &maxOutputSize_bytes);
//	if( retVal != ESP_OK ) cxa_logger_warn(&logger, "get error: %d", retVal);
//	return (retVal == ESP_OK);
//}


//bool cxa_nvsManager_set_cString(const char *const keyIn, char *const valueIn)
//{
//	if( !isInit ) init();
//
//	esp_err_t retVal = nvs_set_str(handle, keyIn, valueIn);
//	if( retVal != ESP_OK ) cxa_logger_warn(&logger, "set error: %d", retVal);
//	return (retVal == ESP_OK);
//}


//bool cxa_nvsManager_get_uint32(const char *const keyIn, uint32_t *const valueOut)
//{
//	if( !isInit ) init();
//
//	esp_err_t retVal = nvs_get_u32(handle, keyIn, valueOut);
//	if( retVal != ESP_OK ) cxa_logger_warn(&logger, "get error: %d", retVal);
//	return (retVal == ESP_OK);
//}


//bool cxa_nvsManager_set_uint32(const char *const keyIn, uint32_t valueIn)
//{
//	if( !isInit ) init();
//
//	esp_err_t retVal = nvs_set_u32(handle, keyIn, valueIn);
//	if( retVal != ESP_OK ) cxa_logger_warn(&logger, "set error: %d", retVal);
//	return (retVal == ESP_OK);
//}

bool cxa_nvsManager_get_uint8(const char *const keyIn, uint8_t *const valueOut)
{
//    if( !isInit ) init();
//
//    esp_err_t retVal = nvs_get_u8(handle, keyIn, valueOut);
//    if( retVal != ESP_OK ) cxa_logger_warn(&logger, "get error: %d", retVal);
//    return (retVal == ESP_OK);
    return 0;
}


bool cxa_nvsManager_set_uint8(const char *const keyIn, uint8_t valueIn)
{
//    if( !isInit ) init();
//
//    esp_err_t retVal = nvs_set_u8(handle, keyIn, valueIn);
//    if( retVal != ESP_OK ) cxa_logger_warn(&logger, "set error: %d", retVal);
//    return (retVal == ESP_OK);
    return 0;
}


bool cxa_nvsManager_get_blob(const char *const keyIn, uint8_t *const valueOut, size_t maxOutputSize_bytesIn, size_t *const actualOutputSize_bytesOut)
{
    if (cxa_stringUtils_equals(keyIn, "nmdLcl_id"))
    {
        AT25LoadMac(); // Could move this out of if's, but other keys may require other functions.
        *valueOut = ((uint8_t) gSysMac.mU.mac.nmdLcl_id);
        // TODO: Check for expected length/value.  Send logger error if unexpected.
    }
    else if (cxa_stringUtils_equals(keyIn, "nmdPrx[1]_id"))
    {
        AT25LoadMac();
        *valueOut = ((uint8_t) gSysMac.mU.mac.nmdPrx1_id);
        // TODO: Check for expected length/value.  Send logger error if unexpected.
    }
    else if (cxa_stringUtils_equals(keyIn, "nmdPrx[1]_mac"))
    {
        AT25LoadMac();
        *valueOut = ((uint8_t) gSysMac.mU.mac.nmdPrx1_mac);
        // TODO: Check for expected length/value.  Send logger error if unexpected.
    }
    else if (cxa_stringUtils_equals(keyIn, "nmdPrx[2]_id"))
    {
        AT25LoadMac();
        *valueOut = ((uint8_t) gSysMac.mU.mac.nmdPrx2_id);
        // TODO: Check for expected length/value.  Send logger error if unexpected.
    }
    else if (cxa_stringUtils_equals(keyIn, "nmdPrx[2]_mac"))
    {
        AT25LoadMac();
        *valueOut = ((uint8_t) gSysMac.mU.mac.nmdPrx2_mac);
        // TODO: Check for expected length/value.  Send logger error if unexpected.
    }
    else if (cxa_stringUtils_equals(keyIn, "nmdPrx[3]_id"))
    {
        AT25LoadMac();
        *valueOut = ((uint8_t) gSysMac.mU.mac.nmdPrx3_id);
        // TODO: Check for expected length/value.  Send logger error if unexpected.
    }
    else if (cxa_stringUtils_equals(keyIn, "nmdPrx[3]_mac"))
    {
        AT25LoadMac();
        *valueOut = ((uint8_t) gSysMac.mU.mac.nmdPrx3_mac);
        // TODO: Check for expected length/value.  Send logger error if unexpected.
    }
    else if (cxa_stringUtils_equals(keyIn, "nmdPrx[4]_id"))
    {
        AT25LoadMac();
        *valueOut = ((uint8_t) gSysMac.mU.mac.nmdPrx4_id);
        // TODO: Check for expected length/value.  Send logger error if unexpected.
    }
    else if (cxa_stringUtils_equals(keyIn, "nmdPrx[4]_mac"))
    {
        AT25LoadMac();
        *valueOut = ((uint8_t) gSysMac.mU.mac.nmdPrx4_mac);
        // TODO: Check for expected length/value.  Send logger error if unexpected.
    }
    else if (cxa_stringUtils_equals(keyIn, "footSnsMac[0]"))
    {
        AT25LoadMac();
        *valueOut = ((uint8_t) gSysMac.mU.mac.footSnsMac0);
        // TODO: Check for expected length/value.  Send logger error if unexpected.
    }
    else if (cxa_stringUtils_equals(keyIn, "footSnsMac[1]"))
    {
        AT25LoadMac();
        *valueOut = ((uint8_t) gSysMac.mU.mac.footSnsMac1);
        // TODO: Check for expected length/value.  Send logger error if unexpected.
    }
    else
    {
        // TODO: Log Error.  Unexpected keyIn.
    }

//	if( !isInit ) init();
//
//	esp_err_t retVal = nvs_get_blob(handle, keyIn, valueOut, &maxOutputSize_bytesIn);
//	if( retVal != ESP_OK )
//	{
//		cxa_logger_warn(&logger, "get error: %d", retVal);
//	}
//	else
//	{
//		if( actualOutputSize_bytesOut != NULL ) *actualOutputSize_bytesOut = maxOutputSize_bytesIn;
//	}
//
//	return (retVal == ESP_OK);
    return 0;
}


bool cxa_nvsManager_set_blob(const char *const keyIn, uint8_t *const valueIn, size_t blobSize_bytesIn)
{
    if (cxa_stringUtils_equals(keyIn, "nmdLcl_id"))
    {
        gSysMac.mU.mac.nmdLcl_id = ((UINT16) *valueIn);
        // TODO: Check for expected length/value.  Send logger error if unexpected.
        AT25StoreMac(); // Could move this out of if's, but other keys may require other functions.
    }
    else if (cxa_stringUtils_equals(keyIn, "nmdPrx[1]_id"))
    {
        gSysMac.mU.mac.nmdPrx1_id = ((UINT16) *valueIn);
        // TODO: Check for expected length/value.  Send logger error if unexpected.
        AT25StoreMac(); // Could move this out of if's, but other keys may require other functions.
    }
    else if (cxa_stringUtils_equals(keyIn, "nmdPrx[1]_mac"))
    {
        gSysMac.mU.mac.nmdPrx1_mac = ((UINT16) *valueIn);
        // TODO: Check for expected length/value.  Send logger error if unexpected.
        AT25StoreMac(); // Could move this out of if's, but other keys may require other functions.
    }
    else if (cxa_stringUtils_equals(keyIn, "nmdPrx[2]_id"))
    {
        gSysMac.mU.mac.nmdPrx2_id = ((UINT16) *valueIn);
        // TODO: Check for expected length/value.  Send logger error if unexpected.
        AT25StoreMac(); // Could move this out of if's, but other keys may require other functions.
    }
    else if (cxa_stringUtils_equals(keyIn, "nmdPrx[2]_mac"))
    {
        gSysMac.mU.mac.nmdPrx2_mac = ((UINT16) *valueIn);
        // TODO: Check for expected length/value.  Send logger error if unexpected.
        AT25StoreMac(); // Could move this out of if's, but other keys may require other functions.
    }
    else if (cxa_stringUtils_equals(keyIn, "nmdPrx[3]_id"))
    {
        gSysMac.mU.mac.nmdPrx3_id = ((UINT16) *valueIn);
        // TODO: Check for expected length/value.  Send logger error if unexpected.
        AT25StoreMac(); // Could move this out of if's, but other keys may require other functions.
    }
    else if (cxa_stringUtils_equals(keyIn, "nmdPrx[3]_mac"))
    {
        gSysMac.mU.mac.nmdPrx3_mac = ((UINT16) *valueIn);
        // TODO: Check for expected length/value.  Send logger error if unexpected.
        AT25StoreMac(); // Could move this out of if's, but other keys may require other functions.
    }
    else if (cxa_stringUtils_equals(keyIn, "nmdPrx[4]_id"))
    {
        gSysMac.mU.mac.nmdPrx4_id = ((UINT16) *valueIn);
        // TODO: Check for expected length/value.  Send logger error if unexpected.
        AT25StoreMac(); // Could move this out of if's, but other keys may require other functions.
    }
    else if (cxa_stringUtils_equals(keyIn, "nmdPrx[4]_mac"))
    {
        gSysMac.mU.mac.nmdPrx4_mac = ((UINT16) *valueIn);
        // TODO: Check for expected length/value.  Send logger error if unexpected.
        AT25StoreMac(); // Could move this out of if's, but other keys may require other functions.
    }
    else if (cxa_stringUtils_equals(keyIn, "footSnsMac[0]"))
    {
        gSysMac.mU.mac.footSnsMac0 = ((UINT16) *valueIn);
        // TODO: Check for expected length/value.  Send logger error if unexpected.
        AT25StoreMac(); // Could move this out of if's, but other keys may require other functions.
    }
    else if (cxa_stringUtils_equals(keyIn, "footSnsMac[1]"))
    {
        gSysMac.mU.mac.footSnsMac1 = ((UINT16) *valueIn);
        // TODO: Check for expected length/value.  Send logger error if unexpected.
        AT25StoreMac(); // Could move this out of if's, but other keys may require other functions.
    }
    else
    {
        // TODO: Log Error.  Unexpected keyIn.
    }

//	if( !isInit ) init();
//
//	esp_err_t retVal = nvs_set_blob(handle, keyIn, valueIn, blobSize_bytesIn);
//	if( retVal != ESP_OK ) cxa_logger_warn(&logger, "set error: %d", retVal);
//	return (retVal == ESP_OK);
    return 0;
}


bool cxa_nvsManager_erase(const char *const keyIn)
{
    UINT16 erasedValue = 0; // Should zero, or something else, be used?

    if (cxa_stringUtils_equals(keyIn, "nmdLcl_id"))
    {
        gSysMac.mU.mac.nmdLcl_id = erasedValue;
        // TODO: Check for expected length/value.  Send logger error if unexpected.
        AT25StoreMac(); // Could move this out of if's, but other keys may require other functions.
    }
    else if (cxa_stringUtils_equals(keyIn, "nmdPrx[1]_id"))
    {
        gSysMac.mU.mac.nmdPrx1_id = erasedValue;
        // TODO: Check for expected length/value.  Send logger error if unexpected.
        AT25StoreMac(); // Could move this out of if's, but other keys may require other functions.
    }
    else if (cxa_stringUtils_equals(keyIn, "nmdPrx[1]_mac"))
    {
        gSysMac.mU.mac.nmdPrx1_mac = erasedValue;
        // TODO: Check for expected length/value.  Send logger error if unexpected.
        AT25StoreMac(); // Could move this out of if's, but other keys may require other functions.
    }
    else if (cxa_stringUtils_equals(keyIn, "nmdPrx[2]_id"))
    {
        gSysMac.mU.mac.nmdPrx2_id = erasedValue;
        // TODO: Check for expected length/value.  Send logger error if unexpected.
        AT25StoreMac(); // Could move this out of if's, but other keys may require other functions.
    }
    else if (cxa_stringUtils_equals(keyIn, "nmdPrx[2]_mac"))
    {
        gSysMac.mU.mac.nmdPrx2_mac = erasedValue;
        // TODO: Check for expected length/value.  Send logger error if unexpected.
        AT25StoreMac(); // Could move this out of if's, but other keys may require other functions.
    }
    else if (cxa_stringUtils_equals(keyIn, "nmdPrx[3]_id"))
    {
        gSysMac.mU.mac.nmdPrx3_id = erasedValue;
        // TODO: Check for expected length/value.  Send logger error if unexpected.
        AT25StoreMac(); // Could move this out of if's, but other keys may require other functions.
    }
    else if (cxa_stringUtils_equals(keyIn, "nmdPrx[3]_mac"))
    {
        gSysMac.mU.mac.nmdPrx3_mac = erasedValue;
        // TODO: Check for expected length/value.  Send logger error if unexpected.
        AT25StoreMac(); // Could move this out of if's, but other keys may require other functions.
    }
    else if (cxa_stringUtils_equals(keyIn, "nmdPrx[4]_id"))
    {
        gSysMac.mU.mac.nmdPrx4_id = erasedValue;
        // TODO: Check for expected length/value.  Send logger error if unexpected.
        AT25StoreMac(); // Could move this out of if's, but other keys may require other functions.
    }
    else if (cxa_stringUtils_equals(keyIn, "nmdPrx[4]_mac"))
    {
        gSysMac.mU.mac.nmdPrx4_mac = erasedValue;
        // TODO: Check for expected length/value.  Send logger error if unexpected.
        AT25StoreMac(); // Could move this out of if's, but other keys may require other functions.
    }
    else if (cxa_stringUtils_equals(keyIn, "footSnsMac[0]"))
    {
        gSysMac.mU.mac.footSnsMac0 = erasedValue;
        // TODO: Check for expected length/value.  Send logger error if unexpected.
        AT25StoreMac(); // Could move this out of if's, but other keys may require other functions.
    }
    else if (cxa_stringUtils_equals(keyIn, "footSnsMac[1]"))
    {
        gSysMac.mU.mac.footSnsMac1 = erasedValue;
        // TODO: Check for expected length/value.  Send logger error if unexpected.
        AT25StoreMac(); // Could move this out of if's, but other keys may require other functions.
    }
    else
    {
        // TODO: Log Error.  Unexpected keyIn.
    }
//	if( !isInit ) init();
//
//	esp_err_t retVal = nvs_erase_key(handle, keyIn);
//	if( retVal != ESP_OK ) cxa_logger_warn(&logger, "erase error: %d", retVal);
//	return (retVal == ESP_OK);
    return 0;
}


//bool cxa_nvsManager_commit(void)
//{
//	if( !isInit ) init();
//
//	esp_err_t retVal = nvs_commit(handle);
//	if( retVal != ESP_OK ) cxa_logger_warn(&logger, "commit error: %d", retVal);
//	return (retVal == ESP_OK);
//}


// ******** local function implementations ********
//static void init(void)
//{
//	if( isInit ) return;
//
//	nvs_flash_init();
//
//	cxa_assert( nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK );
//
//	cxa_logger_init(&logger, "nvsManager");
//
//	isInit = true;
//}
