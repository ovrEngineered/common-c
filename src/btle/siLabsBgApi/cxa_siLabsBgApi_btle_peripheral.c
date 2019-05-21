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


// ******** local type definitions ********


// ******** local function prototypes ********
static cxa_siLabsBgApi_btle_handleCharMapping_t* getMappingEntry_byUuid(cxa_siLabsBgApi_btle_peripheral_t *const btlepIn,
																		cxa_btle_uuid_t* serviceUuidIn,
																		cxa_btle_uuid_t* charUuidIn);

static cxa_siLabsBgApi_btle_handleCharMapping_t* getMappingEntry_byHandle(cxa_siLabsBgApi_btle_peripheral_t *const btlepIn,
																		  uint16_t charHandleIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_siLabsBgApi_btle_peripheral_init(cxa_siLabsBgApi_btle_peripheral_t *const btlepIn, int threadIdIn)
{
	cxa_assert(btlepIn);

	// save our references and setup our internal state
	cxa_array_initStd(&btlepIn->handleCharMap, btlepIn->handleCharMap_raw);
	cxa_array_initStd(&btlepIn->connHandles, btlepIn->connHandles_raw);

	// initialize our super class
	cxa_btle_peripheral_init(&btlepIn->super);
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

	cxa_siLabsBgApi_btle_handleCharMapping_t* targetEntry = getMappingEntry_byUuid(btlepIn, &tmpServiceUuid, &tmpCharUuid);
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
			cxa_eui48_t connectedAddr;
			cxa_eui48_init(&connectedAddr, evt->data.evt_le_connection_opened.address.addr);

			cxa_eui48_string_t connectedAddr_str;
			cxa_eui48_toString(&connectedAddr, &connectedAddr_str);

			cxa_logger_info(&btlepIn->super.logger, "connection from '%s' handle %d", connectedAddr_str.str,
							evt->data.evt_le_connection_opened.connection);

			// store our connection handle
			if( !cxa_array_append(&btlepIn->connHandles, &evt->data.evt_le_connection_opened.connection) )
			{
				cxa_logger_warn(&btlepIn->super.logger, "too many connections");
				return false;
			}

			cxa_btle_peripheral_notify_connectionOpened(&btlepIn->super, &connectedAddr);
			retVal = true;
			break;
		}

		case gecko_evt_le_connection_closed_id:
		{
			// we'll handle all of these events (assuming we're second in line to the central)
			cxa_array_iterate(&btlepIn->connHandles, currHandle, uint8_t)
			{
				if( currHandle == NULL ) continue;

				if( *currHandle == evt->data.evt_le_connection_closed.connection )
				{
					cxa_array_remove(&btlepIn->connHandles, currHandle);
					cxa_logger_info(&btlepIn->super.logger, "connection closed handle %d",
									evt->data.evt_le_connection_closed.connection);
				}
			}

			retVal = true;
			break;
		}

		case gecko_evt_gatt_server_user_write_request_id:
		{
			cxa_siLabsBgApi_btle_handleCharMapping_t* entry = getMappingEntry_byHandle(btlepIn, evt->data.evt_gatt_server_user_write_request.characteristic);
			if( entry != NULL )
			{
				cxa_fixedByteBuffer_t tmpFbb;
				cxa_fixedByteBuffer_init_inPlace(&tmpFbb, evt->data.evt_gatt_server_user_write_request.value.len,
														  evt->data.evt_gatt_server_user_write_request.value.data,
														  evt->data.evt_gatt_server_user_write_request.value.len);
				cxa_btle_peripheral_writeRetVal_t retVal = cxa_btle_peripheral_notify_writeRequest(&btlepIn->super, &entry->serviceUuid, &entry->charUuid, &tmpFbb);

				// I know the last parameter is a weird cast and loses precision...it's just how the BGAPI is written
				gecko_cmd_gatt_server_send_user_write_response(evt->data.evt_gatt_server_user_write_request.connection, evt->data.evt_gatt_server_user_write_request.characteristic, (uint8_t)retVal);
			}
			break;
		}
	}

	return retVal;
}


// ******** local function implementations ********
static cxa_siLabsBgApi_btle_handleCharMapping_t* getMappingEntry_byUuid(cxa_siLabsBgApi_btle_peripheral_t *const btlepIn,
																 cxa_btle_uuid_t* serviceUuidIn,
																 cxa_btle_uuid_t* charUuidIn)
{
	cxa_assert(btlepIn);

	cxa_array_iterate(&btlepIn->handleCharMap, currEntry, cxa_siLabsBgApi_btle_handleCharMapping_t)
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


static cxa_siLabsBgApi_btle_handleCharMapping_t* getMappingEntry_byHandle(cxa_siLabsBgApi_btle_peripheral_t *const btlepIn,
																		  uint16_t charHandleIn)
{
	cxa_assert(btlepIn);

	cxa_array_iterate(&btlepIn->handleCharMap, currEntry, cxa_siLabsBgApi_btle_handleCharMapping_t)
	{
		if( currEntry == NULL ) continue;

		if( currEntry->handle == charHandleIn ) return currEntry;
	}
	// if we made it here, we couldn't find the target entry
	return NULL;
}
