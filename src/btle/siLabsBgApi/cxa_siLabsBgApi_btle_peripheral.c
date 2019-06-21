/*
 * Copyright 2019 ovrEngineered, LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining 
 * a copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
 * copies of the Software, and to permit persons to whom the Software is furnished to 
 * do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all 
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * @author Christopher Armenio
 */
#include "cxa_siLabsBgApi_btle_peripheral.h"


// ******** includes ********
#include <stdbool.h>

#include <cxa_assert.h>

#define CXA_LOG_LEVEL				CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define USER_READ_BUFFER_MAX_BYTES						128


// ******** local type definitions ********


// ******** local function prototypes ********
static cxa_siLabsBgApi_btle_handleCharMapEntry_t* getMappingEntry_byUuid(cxa_siLabsBgApi_btle_peripheral_t *const btlepIn,
																		 cxa_btle_uuid_t* serviceUuidIn,
																		 cxa_btle_uuid_t* charUuidIn);

static cxa_siLabsBgApi_btle_handleCharMapEntry_t* getMappingEntry_byHandle(cxa_siLabsBgApi_btle_peripheral_t *const btlepIn,
																		   uint16_t charHandleIn);

static cxa_siLabsBgApi_btle_handleMacMapEntry_t* getHandleMacEntry_byMac(cxa_siLabsBgApi_btle_peripheral_t *const btlepIn,
																		 cxa_eui48_t *const macAddrIn);

static cxa_siLabsBgApi_btle_handleMacMapEntry_t* getHandleMacEntry_byHandle(cxa_siLabsBgApi_btle_peripheral_t *const btlepIn, uint8_t handleIn);

static void scm_sendNotification(cxa_btle_peripheral_t *const superIn, const char *const serviceUuidStrIn, const char *const characteristicUuidStrIn, cxa_fixedByteBuffer_t *const fbb_dataIn);
static void scm_sendDeferredReadResponse(cxa_btle_peripheral_t *const superIn, cxa_eui48_t *const sourceAddrIn, const char *const serviceUuidStrIn, const char *const characteristicUuidStrIn, cxa_btle_peripheral_readRetVal_t retValIn, cxa_fixedByteBuffer_t *const fbbReadDataIn);
static void scm_sendDeferredWriteResponse(cxa_btle_peripheral_t *const superIn, cxa_eui48_t *const sourceAddrIn, const char *const serviceUuidStrIn, const char *const characteristicUuidStrIn, cxa_btle_peripheral_writeRetVal_t retValIn);
static void scm_setAdvertisingInfo(cxa_btle_peripheral_t *const superIn, uint32_t advertPeriod_msIn, cxa_fixedByteBuffer_t *const fbbAdvertDataIn);
static void scm_startAdvertising(cxa_btle_peripheral_t *const superIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_siLabsBgApi_btle_peripheral_init(cxa_siLabsBgApi_btle_peripheral_t *const btlepIn, int threadIdIn)
{
	cxa_assert(btlepIn);

	// save our references and setup our internal state
	cxa_array_initStd(&btlepIn->handleCharMap, btlepIn->handleCharMap_raw);
	cxa_array_initStd(&btlepIn->handleMacMap, btlepIn->handleMacMap_raw);

	// initialize our super class
	cxa_btle_peripheral_init(&btlepIn->super, scm_sendNotification, scm_sendDeferredReadResponse, scm_sendDeferredWriteResponse, scm_setAdvertisingInfo, scm_startAdvertising);
}


void cxa_siLabsBgApi_btle_peripheral_registerHandle(cxa_siLabsBgApi_btle_peripheral_t *const btlepIn,
													const char *const serviceUuidStrIn,
													const char *const charUuidStrIn,
													uint16_t handleIn)
{
	cxa_assert(btlepIn);

	cxa_btle_uuid_t tmpServiceUuid;
	cxa_btle_uuid_t tmpCharUuid;
	cxa_assert(cxa_btle_uuid_initFromString(&tmpServiceUuid, serviceUuidStrIn));
	cxa_assert(cxa_btle_uuid_initFromString(&tmpCharUuid, charUuidStrIn));

	cxa_siLabsBgApi_btle_handleCharMapEntry_t* targetEntry = getMappingEntry_byUuid(btlepIn, &tmpServiceUuid, &tmpCharUuid);
	if( targetEntry == NULL ) targetEntry = cxa_array_append_empty(&btlepIn->handleCharMap);
	cxa_assert_msg(targetEntry, "increase CXA_BTLE_PERIPHERAL_MAXNUM_CHAR_ENTRIES");

	cxa_btle_uuid_initFromUuid(&targetEntry->serviceUuid, &tmpServiceUuid, false);
	cxa_btle_uuid_initFromUuid(&targetEntry->charUuid, &tmpCharUuid, false);
	targetEntry->handle = handleIn;
}


bool cxa_siLabsBgApi_btle_peripheral_handleBgEvent(cxa_siLabsBgApi_btle_peripheral_t *const btlepIn, struct gecko_cmd_packet *evt)
{
	cxa_assert(btlepIn);

	if( NULL == evt ) return false;

	// Handle events
	bool retVal = false;
	switch( BGLIB_MSG_ID(evt->header) )
	{
		case gecko_evt_le_connection_opened_id:
		{
			// we'll handle all of these events (assuming we're second in line to the central)
			cxa_siLabsBgApi_btle_handleMacMapEntry_t* newEntry = cxa_array_append_empty(&btlepIn->handleMacMap);
			if( newEntry == NULL )
			{
				cxa_logger_warn(&btlepIn->super.logger, "too many connections");
				return false;
			}
			cxa_eui48_init(&newEntry->macAddress, evt->data.evt_le_connection_opened.address.addr);
			newEntry->handle = evt->data.evt_le_connection_opened.connection;

			cxa_eui48_string_t connectedAddr_str;
			cxa_eui48_toString(&newEntry->macAddress, &connectedAddr_str);

			cxa_logger_info(&btlepIn->super.logger, "connection from '%s' handle %d", connectedAddr_str.str,
							evt->data.evt_le_connection_opened.connection);

			cxa_btle_peripheral_notify_connectionOpened(&btlepIn->super, &newEntry->macAddress);
			retVal = true;
			break;
		}

		case gecko_evt_le_connection_closed_id:
		{
			// we'll handle all of these events (assuming we're second in line to the central)
			cxa_siLabsBgApi_btle_handleMacMapEntry_t* macMapEntry = getHandleMacEntry_byHandle(btlepIn, evt->data.evt_le_connection_closed.connection);
			if( macMapEntry != NULL )
			{
				cxa_array_remove(&btlepIn->handleMacMap, macMapEntry);
				cxa_logger_info(&btlepIn->super.logger, "connection closed handle %d",
								evt->data.evt_le_connection_closed.connection);
			}

			retVal = true;
			break;
		}

		case gecko_evt_gatt_server_user_read_request_id:
		{
			cxa_siLabsBgApi_btle_handleCharMapEntry_t* charMapEntry = getMappingEntry_byHandle(btlepIn, evt->data.evt_gatt_server_user_read_request.characteristic);
			cxa_siLabsBgApi_btle_handleMacMapEntry_t* macMapEntry = getHandleMacEntry_byHandle(btlepIn, evt->data.evt_gatt_server_user_read_request.connection);
			if( (charMapEntry != NULL) && (macMapEntry != NULL) )
			{
				cxa_fixedByteBuffer_t tmpFbb;
				uint8_t tmpFbb_raw[USER_READ_BUFFER_MAX_BYTES];
				cxa_fixedByteBuffer_initStd(&tmpFbb, tmpFbb_raw);

				cxa_btle_peripheral_readRetVal_t retVal;
				if( cxa_btle_peripheral_notify_readRequest(&btlepIn->super, &macMapEntry->macAddress, &charMapEntry->serviceUuid, &charMapEntry->charUuid, &tmpFbb, &retVal) )
				{
					// I know the last parameter is a weird cast and loses precision...it's just how the BGAPI is written
					gecko_cmd_gatt_server_send_user_read_response(evt->data.evt_gatt_server_user_read_request.connection, charMapEntry->handle, (uint8_t)retVal, cxa_fixedByteBuffer_getSize_bytes(&tmpFbb), cxa_fixedByteBuffer_get_pointerToStartOfData(&tmpFbb));
				}
			}
			break;
		}

		case gecko_evt_gatt_server_user_write_request_id:
		{
			cxa_siLabsBgApi_btle_handleCharMapEntry_t* charMapEntry = getMappingEntry_byHandle(btlepIn, evt->data.evt_gatt_server_user_write_request.characteristic);
			cxa_siLabsBgApi_btle_handleMacMapEntry_t* macMapEntry = getHandleMacEntry_byHandle(btlepIn, evt->data.evt_gatt_server_user_read_request.connection);
			if( (charMapEntry != NULL) && (macMapEntry != NULL) )
			{
				cxa_fixedByteBuffer_t tmpFbb;
				cxa_fixedByteBuffer_init_inPlace(&tmpFbb, evt->data.evt_gatt_server_user_write_request.value.len,
														  evt->data.evt_gatt_server_user_write_request.value.data,
														  evt->data.evt_gatt_server_user_write_request.value.len);

				cxa_btle_peripheral_writeRetVal_t retVal;
				if( cxa_btle_peripheral_notify_writeRequest(&btlepIn->super, &macMapEntry->macAddress, &charMapEntry->serviceUuid, &charMapEntry->charUuid, &tmpFbb, &retVal) )
				{
					// I know the last parameter is a weird cast and loses precision...it's just how the BGAPI is written
					gecko_cmd_gatt_server_send_user_write_response(evt->data.evt_gatt_server_user_write_request.connection, evt->data.evt_gatt_server_user_write_request.characteristic, (uint8_t)retVal);
				}
			}
			break;
		}
	}

	return retVal;
}


// ******** local function implementations ********
static cxa_siLabsBgApi_btle_handleCharMapEntry_t* getMappingEntry_byUuid(cxa_siLabsBgApi_btle_peripheral_t *const btlepIn,
																 	 	 cxa_btle_uuid_t* serviceUuidIn,
																		 cxa_btle_uuid_t* charUuidIn)
{
	cxa_assert(btlepIn);

	cxa_array_iterate(&btlepIn->handleCharMap, currEntry, cxa_siLabsBgApi_btle_handleCharMapEntry_t)
	{
		if( currEntry == NULL ) continue;

		if( cxa_btle_uuid_isEqual(&currEntry->serviceUuid, serviceUuidIn) &&
			cxa_btle_uuid_isEqual(&currEntry->charUuid, charUuidIn) )
		{
			return currEntry;
		}
	}
	// if we made it here, we couldn't find the target entry
	return NULL;
}


static cxa_siLabsBgApi_btle_handleCharMapEntry_t* getMappingEntry_byHandle(cxa_siLabsBgApi_btle_peripheral_t *const btlepIn,
																		   uint16_t charHandleIn)
{
	cxa_assert(btlepIn);

	cxa_array_iterate(&btlepIn->handleCharMap, currEntry, cxa_siLabsBgApi_btle_handleCharMapEntry_t)
	{
		if( currEntry == NULL ) continue;

		if( currEntry->handle == charHandleIn ) return currEntry;
	}
	// if we made it here, we couldn't find the target entry
	return NULL;
}


static cxa_siLabsBgApi_btle_handleMacMapEntry_t* getHandleMacEntry_byMac(cxa_siLabsBgApi_btle_peripheral_t *const btlepIn,
																		 cxa_eui48_t *const macAddrIn)
{
	cxa_assert(btlepIn);
	cxa_assert(macAddrIn);

	cxa_array_iterate(&btlepIn->handleMacMap, currEntry, cxa_siLabsBgApi_btle_handleMacMapEntry_t)
	{
		if( currEntry == NULL ) continue;

		if( cxa_eui48_isEqual(&currEntry->macAddress, macAddrIn) ) return currEntry;
	}

	return NULL;
}


static cxa_siLabsBgApi_btle_handleMacMapEntry_t* getHandleMacEntry_byHandle(cxa_siLabsBgApi_btle_peripheral_t *const btlepIn, uint8_t handleIn)
{
	cxa_assert(btlepIn);

	cxa_array_iterate(&btlepIn->handleMacMap, currEntry, cxa_siLabsBgApi_btle_handleMacMapEntry_t)
	{
		if( currEntry == NULL ) continue;

		if( currEntry->handle == handleIn ) return currEntry;
	}

	return NULL;
}


static void scm_sendNotification(cxa_btle_peripheral_t *const superIn, const char *const serviceUuidStrIn, const char *const characteristicUuidStrIn, cxa_fixedByteBuffer_t *const fbb_dataIn)
{
	cxa_siLabsBgApi_btle_peripheral_t *const btlepIn = (cxa_siLabsBgApi_btle_peripheral_t *const)superIn;
	cxa_assert(btlepIn);

	// convert our uuids
	cxa_btle_uuid_t tmpServiceUuid;
	cxa_btle_uuid_t tmpCharUuid;
	cxa_assert(cxa_btle_uuid_initFromString(&tmpServiceUuid, serviceUuidStrIn));
	cxa_assert(cxa_btle_uuid_initFromString(&tmpCharUuid, characteristicUuidStrIn));

	cxa_siLabsBgApi_btle_handleCharMapEntry_t* charMapEntry = getMappingEntry_byUuid(btlepIn, &tmpServiceUuid, &tmpCharUuid);
	if( charMapEntry != NULL )
	{
		// 0xFF is all devices
		gecko_cmd_gatt_server_send_characteristic_notification(0xFF, charMapEntry->handle, cxa_fixedByteBuffer_getSize_bytes(fbb_dataIn), cxa_fixedByteBuffer_get_pointerToStartOfData(fbb_dataIn));
	}
}


static void scm_sendDeferredReadResponse(cxa_btle_peripheral_t *const superIn, cxa_eui48_t *const sourceAddrIn, const char *const serviceUuidStrIn, const char *const characteristicUuidStrIn, cxa_btle_peripheral_readRetVal_t retValIn, cxa_fixedByteBuffer_t *const fbbReadDataIn)
{
	cxa_siLabsBgApi_btle_peripheral_t *const btlepIn = (cxa_siLabsBgApi_btle_peripheral_t *const)superIn;
	cxa_assert(btlepIn);

	// convert our uuids
	cxa_btle_uuid_t tmpServiceUuid;
	cxa_btle_uuid_t tmpCharUuid;
	cxa_assert(cxa_btle_uuid_initFromString(&tmpServiceUuid, serviceUuidStrIn));
	cxa_assert(cxa_btle_uuid_initFromString(&tmpCharUuid, characteristicUuidStrIn));

	cxa_siLabsBgApi_btle_handleCharMapEntry_t* charMapEntry = getMappingEntry_byUuid(btlepIn, &tmpServiceUuid, &tmpCharUuid);
	cxa_siLabsBgApi_btle_handleMacMapEntry_t* macMapEntry = getHandleMacEntry_byMac(btlepIn, sourceAddrIn);
	if( (charMapEntry != NULL) && (macMapEntry != NULL) )
	{
		// I know the last parameter is a weird cast and loses precision...it's just how the BGAPI is written
		gecko_cmd_gatt_server_send_user_read_response(macMapEntry->handle, charMapEntry->handle, (uint8_t)retValIn, cxa_fixedByteBuffer_getSize_bytes(fbbReadDataIn), cxa_fixedByteBuffer_get_pointerToStartOfData(fbbReadDataIn));
	}
}


static void scm_sendDeferredWriteResponse(cxa_btle_peripheral_t *const superIn, cxa_eui48_t *const sourceAddrIn, const char *const serviceUuidStrIn, const char *const characteristicUuidStrIn, cxa_btle_peripheral_writeRetVal_t retValIn)
{
	cxa_siLabsBgApi_btle_peripheral_t *const btlepIn = (cxa_siLabsBgApi_btle_peripheral_t *const)superIn;
	cxa_assert(btlepIn);

	// convert our uuids
	cxa_btle_uuid_t tmpServiceUuid;
	cxa_btle_uuid_t tmpCharUuid;
	cxa_assert(cxa_btle_uuid_initFromString(&tmpServiceUuid, serviceUuidStrIn));
	cxa_assert(cxa_btle_uuid_initFromString(&tmpCharUuid, characteristicUuidStrIn));

	cxa_siLabsBgApi_btle_handleCharMapEntry_t* charMapEntry = getMappingEntry_byUuid(btlepIn, &tmpServiceUuid, &tmpCharUuid);
	cxa_siLabsBgApi_btle_handleMacMapEntry_t* macMapEntry = getHandleMacEntry_byMac(btlepIn, sourceAddrIn);
	if( (charMapEntry != NULL) && (macMapEntry != NULL) )
	{
		// I know the last parameter is a weird cast and loses precision...it's just how the BGAPI is written
		gecko_cmd_gatt_server_send_user_write_response(macMapEntry->handle, charMapEntry->handle, (uint8_t)retValIn);
	}
}


static void scm_setAdvertisingInfo(cxa_btle_peripheral_t *const superIn, uint32_t advertPeriod_msIn, cxa_fixedByteBuffer_t *const fbbAdvertDataIn)
{
	cxa_siLabsBgApi_btle_peripheral_t *const btlepIn = (cxa_siLabsBgApi_btle_peripheral_t *const)superIn;
	cxa_assert(btlepIn);

	// set our advertising timing
	struct gecko_msg_le_gap_set_advertise_timing_rsp_t* rsp_advTiming = gecko_cmd_le_gap_set_advertise_timing(0, (advertPeriod_msIn * 1600) / 1000, ((advertPeriod_msIn + (advertPeriod_msIn/2)) * 1600) / 1000, 0, 0);
	if( rsp_advTiming->result != 0 )
	{
		cxa_logger_warn(&btlepIn->super.logger, "error setting advertising timing: %d", rsp_advTiming->result);
		return;
	}

	// set our advertising data (0 - advertising)
	struct gecko_msg_le_gap_bt5_set_adv_data_rsp_t* rsp_advData = gecko_cmd_le_gap_bt5_set_adv_data(0, 0,
																	(fbbAdvertDataIn != NULL) ? cxa_fixedByteBuffer_getSize_bytes(fbbAdvertDataIn) : 0,
																	(fbbAdvertDataIn != NULL) ? cxa_fixedByteBuffer_get_pointerToStartOfData(fbbAdvertDataIn) : NULL);
	if( rsp_advData->result != 0 )
	{
		cxa_logger_warn(&btlepIn->super.logger, "error setting advertising data: %d", rsp_advData->result);
		return;
	}
}


static void scm_startAdvertising(cxa_btle_peripheral_t *const superIn)
{
	cxa_siLabsBgApi_btle_peripheral_t *const btlepIn = (cxa_siLabsBgApi_btle_peripheral_t *const)superIn;
	cxa_assert(btlepIn);

	cxa_logger_debug(&btlepIn->super.logger, "starting advertising");

	// start advertising
	struct gecko_msg_le_gap_start_advertising_rsp_t* rsp_advStart = gecko_cmd_le_gap_start_advertising(0, le_gap_general_discoverable, le_gap_connectable_scannable);
	if( rsp_advStart->result != 0 )
	{
		cxa_logger_warn(&btlepIn->super.logger, "error starting advertising: %d", rsp_advStart->result);
		return;
	}
}
