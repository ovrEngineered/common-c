/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_esp32_btle_peripheral.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_esp32_btle_module.h>
#include <cxa_fixedByteBuffer.h>
#include <cxa_stringUtils.h>

#include <string.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define MAX_SIZE_ADVERT_DATA			25


// ******** local type definitions ********


// ******** local function prototypes ********
static void createServiceForCurrCharEntryIndex(cxa_esp32_btle_peripheral_t *const btlepIn, esp_gatt_if_t gatts_if);
static void createCharacteristicForCurrCharEntryIndex(cxa_esp32_btle_peripheral_t *const btlepIn, uint16_t serviceHandleIn);
static void handleRead(cxa_esp32_btle_peripheral_t *const btlepIn, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void handleWrite(cxa_esp32_btle_peripheral_t *const btlepIn, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

static void scm_sendNotification(cxa_btle_peripheral_t *const superIn, const char *const serviceUuidStrIn, const char *const characteristicUuidStrIn, cxa_fixedByteBuffer_t *const fbb_dataIn);
static void scm_sendDeferredReadResponse(cxa_btle_peripheral_t *const superIn, cxa_eui48_t *const sourceAddrIn, const char *const serviceUuidStrIn, const char *const characteristicUuidStrIn, cxa_btle_peripheral_readRetVal_t retValIn, cxa_fixedByteBuffer_t *const fbbReadDataIn);
static void scm_sendDeferredWriteResponse(cxa_btle_peripheral_t *const superIn, cxa_eui48_t *const sourceAddrIn, const char *const serviceUuidStrIn, const char *const characteristicUuidStrIn, cxa_btle_peripheral_writeRetVal_t retValIn);
static void scm_setAdvertisingInfo(cxa_btle_peripheral_t *const superIn, uint32_t advertPeriod_msIn, cxa_fixedByteBuffer_t *const fbbAdvertDataIn);
static void scm_startAdvertising(cxa_btle_peripheral_t *const superIn);


// ********  local variable declarations *********



// ******** global function implementations ********
void cxa_esp32_btle_peripheral_init(cxa_esp32_btle_peripheral_t *const btlepIn, int threadIdIn)
{
	cxa_assert(btlepIn);

	// initialize our super class
	cxa_btle_peripheral_init(&btlepIn->super, scm_sendNotification, scm_sendDeferredReadResponse, scm_sendDeferredWriteResponse, scm_setAdvertisingInfo, scm_startAdvertising);

	// setup our internal state
	cxa_array_initStd(&btlepIn->handleCharMap, btlepIn->handleCharMap_raw);

	// setup our advertising information
	btlepIn->advParams.adv_int_min        = 0x20;
	btlepIn->advParams.adv_int_max        = 0x40;
	btlepIn->advParams.adv_type           = ADV_TYPE_IND;
	btlepIn->advParams.own_addr_type      = BLE_ADDR_TYPE_PUBLIC;
	btlepIn->advParams.channel_map        = ADV_CHNL_ALL;
	btlepIn->advParams.adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;

	btlepIn->advData.set_scan_rsp = false;
	btlepIn->advData.include_name = false;
	btlepIn->advData.include_txpower = false;
	btlepIn->advData.min_interval = 0xFFFF;
	btlepIn->advData.max_interval = 0xFFFF;
	btlepIn->advData.appearance = 0x00;
	btlepIn->advData.manufacturer_len = 0;
	btlepIn->advData.p_manufacturer_data = NULL;
	btlepIn->advData.service_data_len = 0;
	btlepIn->advData.p_service_data = NULL;
	btlepIn->advData.service_uuid_len = 0;
	btlepIn->advData.p_service_uuid = NULL;
	btlepIn->advData.flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
	cxa_fixedByteBuffer_initStd(&btlepIn->fbb_advertData_manSpecific, btlepIn->fbb_advertData_manSpecific_raw);
}


void cxa_esp32_btle_peripheral_setDeviceName(cxa_esp32_btle_peripheral_t *const btlepIn, const char *const nameIn)
{
	cxa_assert(btlepIn);
	cxa_assert(nameIn);

	esp_ble_gap_set_device_name(nameIn);
}


void cxa_esp32_btle_peripheral_handleEvent_gap(cxa_esp32_btle_peripheral_t *const btlepIn, esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
	cxa_assert(btlepIn);
}


void cxa_esp32_btle_peripheral_handleEvent_gatts(cxa_esp32_btle_peripheral_t *const btlepIn, esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
	cxa_assert(btlepIn);

	switch( event )
	{
		case ESP_GATTS_REG_EVT:
		{
			// start our service registration
			cxa_logger_debug(&btlepIn->super.logger, "starting gatt service registration");
			btlepIn->currRegisteringCharEntryIndex = 0;
			createServiceForCurrCharEntryIndex(btlepIn, gatts_if);
			break;
		}

		case ESP_GATTS_CREATE_EVT:
		{
			if( param->create.status != ESP_GATT_OK )
			{
				cxa_logger_warn(&btlepIn->super.logger, "cannot create service: %d", param->create.status);
				return;
			}

			// service was created, record our service handle
			cxa_btle_peripheral_charEntry_t* currCharEntry = cxa_array_get(&btlepIn->super.charEntries, btlepIn->currRegisteringCharEntryIndex);
			if( currCharEntry == NULL ) return;
			cxa_logger_debug(&btlepIn->super.logger, "service %s -> handle %d", currCharEntry->serviceUuid_str, param->create.service_handle);

			cxa_logger_debug(&btlepIn->super.logger, "starting service handle %d", param->create.service_handle);
			esp_ble_gatts_start_service(param->create.service_handle);
			break;
		}

		case ESP_GATTS_ADD_CHAR_EVT:
		{
			if( param->add_char.status != ESP_GATT_OK )
			{
				cxa_logger_warn(&btlepIn->super.logger, "cannot create char: %d", param->add_char.status);
				return;
			}

			// characteristic was created, record our char handle
			cxa_btle_peripheral_charEntry_t* currCharEntry = cxa_array_get(&btlepIn->super.charEntries, btlepIn->currRegisteringCharEntryIndex);
			if( currCharEntry == NULL ) return;

			// add to our lookup map
			cxa_esp32_btle_handleCharMapEntry_t newEntry = { .charEntry = currCharEntry, .handle_service = param->add_char.service_handle, .handle_char = param->add_char.attr_handle };
			cxa_assert( cxa_array_append(&btlepIn->handleCharMap, &newEntry) );
			cxa_logger_debug(&btlepIn->super.logger, "char %s -> handle %d", currCharEntry->charUuid_str, param->add_char.attr_handle);

			// move onto the next char map entry
			btlepIn->currRegisteringCharEntryIndex++;
			createServiceForCurrCharEntryIndex(btlepIn, gatts_if);
			break;
		}

		case ESP_GATTS_START_EVT:
		{
			if( param->start.status != ESP_GATT_OK )
			{
				cxa_logger_warn(&btlepIn->super.logger, "cannot start service: %d", param->start.service_handle);
				return;
			}
			cxa_logger_debug(&btlepIn->super.logger, "service handle %d started", param->start.service_handle);

			// start creating the corresponding characteristics
			createCharacteristicForCurrCharEntryIndex(btlepIn, param->start.service_handle);
			break;
		}

		case ESP_GATTS_READ_EVT:
		{
			cxa_logger_debug(&btlepIn->super.logger, "read handle %d", param->read.handle);
			handleRead(btlepIn, gatts_if, param);
			break;
		}

		case ESP_GATTS_WRITE_EVT:
		{
			cxa_logger_debug(&btlepIn->super.logger, "write handle %d", param->read.handle);
			handleWrite(btlepIn, gatts_if, param);
			break;
		}

		default:
			break;
	}
}


// ******** local function implementations ********
static void createServiceForCurrCharEntryIndex(cxa_esp32_btle_peripheral_t *const btlepIn, esp_gatt_if_t gatts_if)
{
	cxa_assert(btlepIn);

	// start with our current entry
	cxa_btle_peripheral_charEntry_t* currCharEntry = cxa_array_get(&btlepIn->super.charEntries, btlepIn->currRegisteringCharEntryIndex);
	if( currCharEntry == NULL )
	{
		cxa_logger_debug(&btlepIn->super.logger, "done registering services");
		return;
	}
	cxa_btle_uuid_t currServiceUuid;
	if( !cxa_btle_uuid_initFromString(&currServiceUuid, currCharEntry->serviceUuid_str) ) return;

	// iterate through other entries to see if we've registered this service before
	bool createNewService = true;
	for( size_t i = 0; i < btlepIn->currRegisteringCharEntryIndex; i++ )
	{
		cxa_btle_peripheral_charEntry_t* prevCharEntry = cxa_array_get(&btlepIn->super.charEntries, i);

		if( cxa_btle_uuid_isEqualToString(&currServiceUuid, prevCharEntry->serviceUuid_str) )
		{
			createNewService = false;
			break;
		}
	}

	// create a new service if we need to
	if( createNewService )
	{
		// calculate how many characteristics we need to reserve
		size_t numCharsForService = 0;
		cxa_array_iterate(&btlepIn->super.charEntries, countCharEntry, cxa_btle_peripheral_charEntry_t)
		{
			if( countCharEntry == NULL ) continue;

			if( cxa_btle_uuid_isEqualToString(&currServiceUuid, countCharEntry->serviceUuid_str) )
			{
				numCharsForService++;
			}
		}

		// create the service
		esp_gatt_srvc_id_t srv;
		srv.is_primary = true;
		if( currServiceUuid.type == CXA_BTLE_UUID_TYPE_128BIT )
		{
			srv.id.uuid.len = ESP_UUID_LEN_128;
			for( size_t i = 0; i < sizeof(srv.id.uuid.uuid.uuid128); i++ )
			{
				srv.id.uuid.uuid.uuid128[i] = currServiceUuid.as128Bit.bytes[sizeof(srv.id.uuid.uuid.uuid128)-i-1];
			}
		}
		else
		{
			srv.id.uuid.len = ESP_UUID_LEN_16;
			memcpy(&srv.id.uuid.uuid.uuid16, &currServiceUuid.as16Bit, sizeof(srv.id.uuid.uuid.uuid16));
		}
		cxa_logger_debug(&btlepIn->super.logger, "creating service %s with %d chars", currCharEntry->serviceUuid_str, numCharsForService);
		esp_ble_gatts_create_service(gatts_if, &srv, 2+(2*numCharsForService));
	}
	else
	{
		// we already have this service, skip to creating the characteristic

		// first, we need to lookup the service handle
		uint16_t existingServiceHandle = 0;
		cxa_array_iterate(&btlepIn->handleCharMap, currCharMapEntry, cxa_esp32_btle_handleCharMapEntry_t)
		{
			if( currCharMapEntry == NULL ) continue;

			if( cxa_stringUtils_equals_ignoreCase(currCharEntry->serviceUuid_str, currCharMapEntry->charEntry->serviceUuid_str) )
			{
				existingServiceHandle = currCharMapEntry->handle_service;
				break;
			}
		}
		if( existingServiceHandle == 0 )
		{
			cxa_logger_warn(&btlepIn->super.logger, "failed to find service handle for existing service '%s'", currCharEntry->serviceUuid_str);
			return;
		}

		// if we made it here, we found our existing service
		createCharacteristicForCurrCharEntryIndex(btlepIn, existingServiceHandle);
	}
}


static void createCharacteristicForCurrCharEntryIndex(cxa_esp32_btle_peripheral_t *const btlepIn, uint16_t serviceHandleIn)
{
	cxa_assert(btlepIn);

	// get our current characteristic entry
	cxa_btle_peripheral_charEntry_t* currCharEntry = cxa_array_get(&btlepIn->super.charEntries, btlepIn->currRegisteringCharEntryIndex);
	cxa_assert(currCharEntry);
	cxa_btle_uuid_t currCharUuid;
	if( !cxa_btle_uuid_initFromString(&currCharUuid, currCharEntry->charUuid_str) ) return;

	// determine the permissions
	esp_gatt_perm_t permissions = (((currCharEntry->cbs.onReadRequest != NULL) || (currCharEntry->cbs.onDeferredReadRequest != NULL)) ? ESP_GATT_PERM_READ : 0) |
								  (((currCharEntry->cbs.onWriteRequest != NULL) || (currCharEntry->cbs.onDeferredWriteRequest != NULL)) ? ESP_GATT_PERM_WRITE : 0);

	// determine the properties
	esp_gatt_char_prop_t properties = (((currCharEntry->cbs.onReadRequest != NULL) || (currCharEntry->cbs.onDeferredReadRequest != NULL)) ? ESP_GATT_CHAR_PROP_BIT_READ : 0) |
			  	  	  	  	  	  	  (((currCharEntry->cbs.onWriteRequest != NULL) || (currCharEntry->cbs.onDeferredWriteRequest != NULL)) ? ESP_GATT_CHAR_PROP_BIT_WRITE : 0) |
									  (currCharEntry->allowNotifications ? ESP_GATT_CHAR_PROP_BIT_NOTIFY : 0) |
									  (currCharEntry->allowIndications ? ESP_GATT_CHAR_PROP_BIT_INDICATE : 0);

	// convert our uuid
	esp_bt_uuid_t charUuid;
	if( currCharUuid.type == CXA_BTLE_UUID_TYPE_128BIT )
	{
		charUuid.len = ESP_UUID_LEN_128;
		for( size_t i = 0; i < sizeof(charUuid.uuid.uuid128); i++ )
		{
			charUuid.uuid.uuid128[i] = currCharUuid.as128Bit.bytes[sizeof(charUuid.uuid.uuid128)-i-1];
		}
	}
	else
	{
		charUuid.len = ESP_UUID_LEN_16;
		memcpy(&charUuid.uuid.uuid16, &currCharUuid.as16Bit, sizeof(charUuid.uuid.uuid16));
	}

	// if we made it here, we have all of the information we need
    cxa_logger_debug(&btlepIn->super.logger, "creating char %s for service %d", currCharEntry->charUuid_str, serviceHandleIn);
    esp_ble_gatts_add_char(serviceHandleIn, &charUuid, permissions, properties, NULL, NULL);
}


static void handleRead(cxa_esp32_btle_peripheral_t *const btlepIn, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
	// get our response ready
	esp_gatt_rsp_t rsp;
	memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
	rsp.attr_value.handle = param->read.handle;
	rsp.attr_value.len = 0;
	cxa_fixedByteBuffer_t readData_fbb;
	cxa_fixedByteBuffer_initStd(&readData_fbb, rsp.attr_value.value);

	// see if we have any registered handlers
	cxa_btle_peripheral_readRetVal_t retVal = CXA_BTLE_PERIPHERAL_READRET_SUCCESS;
	cxa_array_iterate(&btlepIn->handleCharMap, currCharMapEntry, cxa_esp32_btle_handleCharMapEntry_t)
	{
		if( currCharMapEntry == NULL ) continue;

		if( currCharMapEntry->handle_char == param->read.handle )
		{
			if( currCharMapEntry->charEntry->cbs.onReadRequest != NULL )
			{
				retVal = currCharMapEntry->charEntry->cbs.onReadRequest(&readData_fbb, currCharMapEntry->charEntry->cbs.userVar_read);
				if( retVal == CXA_BTLE_PERIPHERAL_READRET_SUCCESS ) rsp.attr_value.len = cxa_fixedByteBuffer_getSize_bytes(&readData_fbb);
			}
			else if( currCharMapEntry->charEntry->cbs.onDeferredReadRequest != NULL ) cxa_assert_failWithMsg("not supported");
			break;
		}
	}

	// translate our return value
	esp_gatt_status_t espRet = ESP_GATT_OK;
	switch( retVal )
	{
		case CXA_BTLE_PERIPHERAL_READRET_SUCCESS:
			espRet = ESP_GATT_OK;
			break;

		case CXA_BTLE_PERIPHERAL_READRET_NOT_PERMITTED:
			espRet = ESP_GATT_READ_NOT_PERMIT;
			break;

		case CXA_BTLE_PERIPHERAL_READRET_VALUE_NOT_ALLOWED:
			espRet = ESP_GATT_OUT_OF_RANGE;
			break;

		case CXA_BTLE_PERIPHERAL_READRET_ATTRIBUTE_NOT_FOUND:
			espRet = ESP_GATT_INVALID_HANDLE;
			break;

		case CXA_BTLE_PERIPHERAL_READRET_UNLIKELY:
			espRet = ESP_GATT_UNKNOWN_ERROR;
			break;
	}

	// send our return value
	esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, espRet, &rsp);
}


static void handleWrite(cxa_esp32_btle_peripheral_t *const btlepIn, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
	// get our response ready
	esp_gatt_rsp_t rsp;
	memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
	rsp.attr_value.handle = param->write.handle;
	rsp.attr_value.len = param->write.len;
	rsp.attr_value.offset = param->write.offset;
	rsp.attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
	memcpy(rsp.attr_value.value, param->write.value, param->write.len);

	cxa_fixedByteBuffer_t writeData_fbb;
	cxa_fixedByteBuffer_init_inPlace(&writeData_fbb, param->write.len, param->write.value, param->write.len);

	// see if we have any registered handlers
	cxa_btle_peripheral_writeRetVal_t retVal = CXA_BTLE_PERIPHERAL_WRITERET_SUCCESS;
	cxa_array_iterate(&btlepIn->handleCharMap, currCharMapEntry, cxa_esp32_btle_handleCharMapEntry_t)
	{
		if( currCharMapEntry == NULL ) continue;

		if( currCharMapEntry->handle_char == param->read.handle )
		{
			if( currCharMapEntry->charEntry->cbs.onWriteRequest != NULL )
			{
				retVal = currCharMapEntry->charEntry->cbs.onWriteRequest(&writeData_fbb, currCharMapEntry->charEntry->cbs.userVar_write);
			}
			else if( currCharMapEntry->charEntry->cbs.onDeferredWriteRequest != NULL ) cxa_assert_failWithMsg("not supported");
			break;
		}
	}

	// translate our return value
	esp_gatt_status_t espRet = ESP_GATT_OK;
	switch( retVal )
	{
		case CXA_BTLE_PERIPHERAL_WRITERET_SUCCESS:
			espRet = ESP_GATT_OK;
			break;

		case CXA_BTLE_PERIPHERAL_WRITERET_NOT_PERMITTED:
			espRet = ESP_GATT_WRITE_NOT_PERMIT;
			break;

		case CXA_BTLE_PERIPHERAL_WRITERET_VALUE_NOT_ALLOWED:
			espRet = ESP_GATT_OUT_OF_RANGE;
			break;

		case CXA_BTLE_PERIPHERAL_WRITERET_ATTRIBUTE_NOT_FOUND:
			espRet = ESP_GATT_INVALID_HANDLE;
			break;

		case CXA_BTLE_PERIPHERAL_WRITERET_UNLIKELY:
			espRet = ESP_GATT_UNKNOWN_ERROR;
			break;
	}

	// send our return value
	esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, espRet, &rsp);
}


static void scm_sendNotification(cxa_btle_peripheral_t *const superIn, const char *const serviceUuidStrIn, const char *const characteristicUuidStrIn, cxa_fixedByteBuffer_t *const fbb_dataIn)
{
	cxa_esp32_btle_peripheral_t *const btlepIn = (cxa_esp32_btle_peripheral_t *const)superIn;
	cxa_assert(btlepIn);
}


static void scm_sendDeferredReadResponse(cxa_btle_peripheral_t *const superIn, cxa_eui48_t *const sourceAddrIn, const char *const serviceUuidStrIn, const char *const characteristicUuidStrIn, cxa_btle_peripheral_readRetVal_t retValIn, cxa_fixedByteBuffer_t *const fbbReadDataIn)
{
	cxa_esp32_btle_peripheral_t *const btlepIn = (cxa_esp32_btle_peripheral_t *const)superIn;
	cxa_assert(btlepIn);
}


static void scm_sendDeferredWriteResponse(cxa_btle_peripheral_t *const superIn, cxa_eui48_t *const sourceAddrIn, const char *const serviceUuidStrIn, const char *const characteristicUuidStrIn, cxa_btle_peripheral_writeRetVal_t retValIn)
{
	cxa_esp32_btle_peripheral_t *const btlepIn = (cxa_esp32_btle_peripheral_t *const)superIn;
	cxa_assert(btlepIn);
}


static void scm_setAdvertisingInfo(cxa_btle_peripheral_t *const superIn, uint32_t advertPeriod_msIn, cxa_fixedByteBuffer_t *const fbbAdvertDataIn)
{
	cxa_esp32_btle_peripheral_t *const btlepIn = (cxa_esp32_btle_peripheral_t *const)superIn;
	cxa_assert(btlepIn);

	// set our timing parameters
	btlepIn->advParams.adv_int_min = (advertPeriod_msIn - (advertPeriod_msIn/2.0)) * 0.625;
	btlepIn->advParams.adv_int_max = (advertPeriod_msIn + (advertPeriod_msIn/2.0)) * 0.625;

	// we need to parse the advert data coming in so we can split it out into the ESP32-specific structure
	uint8_t fieldLen_bytes;
	uint8_t fieldType_raw;
	for( size_t i = 0; i < cxa_fixedByteBuffer_getSize_bytes(fbbAdvertDataIn); i++ )
	{
		if( !cxa_fixedByteBuffer_get_uint8(fbbAdvertDataIn, i++, fieldLen_bytes) ) break;
		if( !cxa_fixedByteBuffer_get_uint8(fbbAdvertDataIn, i++, fieldType_raw) ) break;

		switch( fieldType_raw )
		{
			case 0x01:
			{
				// flags field
				uint8_t field_flags;
				if( !cxa_fixedByteBuffer_get_uint8(fbbAdvertDataIn, i, field_flags) ) break;
				btlepIn->advData.flag = field_flags;
				break;
			}

			case 0xFF:
			{
				uint8_t* manAdvData = cxa_fixedByteBuffer_get_pointerToIndex(fbbAdvertDataIn, i);
				if( manAdvData == NULL ) break;
				btlepIn->advData.manufacturer_len = fieldLen_bytes - 1;
				btlepIn->advData.p_manufacturer_data = manAdvData;

				i += btlepIn->advData.manufacturer_len;
				break;
			}

			default:
				cxa_logger_warn(&btlepIn->super.logger, "unknown advertisement field type: %d", fieldType_raw);
				break;
		}
	}

	if( esp_ble_gap_config_adv_data(&btlepIn->advData) != ESP_OK )
	{
		cxa_logger_warn(&btlepIn->super.logger, "invalid advData");
	}
}


static void scm_startAdvertising(cxa_btle_peripheral_t *const superIn)
{
	cxa_esp32_btle_peripheral_t *const btlepIn = (cxa_esp32_btle_peripheral_t *const)superIn;
	cxa_assert(btlepIn);

	esp_ble_gap_start_advertising(&btlepIn->advParams);
}
