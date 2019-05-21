/**
 * @copyright 2019 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Christopher Armenio
 */
#include "cxa_siLabsBgApi_btle_central.h"


// ******** includes ********
#include <gecko_bglib.h>

#include <cxa_array.h>
#include <cxa_assert.h>
#include <cxa_ioStream_peekable.h>
#include <cxa_runLoop.h>
#include <cxa_siLabsBgApi_module.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_DEBUG
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static cxa_siLabsBgApi_btle_connection_t* getUnusedConnection(cxa_siLabsBgApi_btle_central_t *const btlecIn);
static cxa_siLabsBgApi_btle_connection_t* getConnectionByAddress(cxa_siLabsBgApi_btle_central_t *const btlecIn, cxa_eui48_t *const targetAddrIn);
static cxa_siLabsBgApi_btle_connection_t* getConnectionByHandle(cxa_siLabsBgApi_btle_central_t *const btlecIn, uint8_t connHandleIn);

static cxa_btle_central_state_t scm_getState(cxa_btle_central_t *const superIn);
static void scm_startScan(cxa_btle_central_t *const superIn, bool isActiveIn);
static void scm_stopScan(cxa_btle_central_t *const superIn);
static void scm_startConnection(cxa_btle_central_t *const superIn, cxa_eui48_t *const targetAddrIn, bool isRandomAddrIn);
static void scm_stopConnection(cxa_btle_central_t *const superIn, cxa_eui48_t *const targetAddrIn);
static void scm_readFromCharacteristic(cxa_btle_central_t *const superIn,
									   cxa_eui48_t *const targetAddrIn,
									   const char *const serviceUuidIn,
									   const char *const characteristicUuidIn);
static void scm_writeToCharacteristic(cxa_btle_central_t *const superIn,
									  cxa_eui48_t *const targetAddrIn,
									  const char *const serviceIdIn,
									  const char *const characteristicIdIn,
									  cxa_fixedByteBuffer_t *const dataIn);
static void scm_changeNotifications(cxa_btle_central_t *const superIn,
		  	  	  	  	  	  	    cxa_eui48_t *const targetAddrIn,
									const char *const serviceUuidIn,
									const char *const characteristicUuidIn,
									bool enableNotifications);

static void runLoopOneShot_startScan(void* userVarIn);
static void runLoopOneShot_stopScan(void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_siLabsBgApi_btle_central_init(cxa_siLabsBgApi_btle_central_t *const btlecIn, int threadIdIn)
{
	cxa_assert(btlecIn);

	// save our references and setup our internal state
	btlecIn->threadId = threadIdIn;

	// initialize our connections
	for( size_t i = 0; i < sizeof(btlecIn->conns)/sizeof(*btlecIn->conns); i++ )
	{
		cxa_siLabsBgApi_btle_connection_init(&btlecIn->conns[i], &btlecIn->super, threadIdIn);
	}

	// initialize our super class
	cxa_btle_central_init(&btlecIn->super, scm_getState, scm_startScan, scm_stopScan, scm_startConnection, scm_stopConnection, scm_readFromCharacteristic, scm_writeToCharacteristic, scm_changeNotifications);
}


bool cxa_siLabsBgApi_btle_central_handleBgEvent(cxa_siLabsBgApi_btle_central_t *const btlecIn, struct gecko_cmd_packet *evt)
{
	cxa_assert(btlecIn);

	if( NULL == evt ) return false;

	// Handle events
	bool retVal = false;
	switch( BGLIB_MSG_ID(evt->header) )
	{
		case gecko_evt_le_connection_opened_id:
		{
			cxa_eui48_t connectedAddr;
			cxa_eui48_init(&connectedAddr, evt->data.evt_le_connection_opened.address.addr);
			// update our connection handle
			cxa_siLabsBgApi_btle_connection_t* currConn = getConnectionByAddress(btlecIn, &connectedAddr);
			if( currConn != NULL )
			{
				cxa_logger_debug(&btlecIn->super.logger, "connected");
				cxa_siLabsBgApi_btle_connection_handleEvent_opened(currConn, evt->data.evt_le_connection_opened.connection);
				retVal = true;
			}
			break;
		}

		case gecko_evt_le_connection_closed_id:
		{
			cxa_siLabsBgApi_btle_connection_t* currConn = getConnectionByHandle(btlecIn, evt->data.evt_le_connection_closed.connection);
			if( currConn != NULL )
			{
				cxa_logger_debug(&btlecIn->super.logger, "disconnected");
				cxa_siLabsBgApi_btle_connection_handleEvent_closed(currConn, evt->data.evt_le_connection_closed.reason);
				retVal = true;
			}
			break;
		}

		case gecko_evt_gatt_procedure_completed_id:
		{
			cxa_siLabsBgApi_btle_connection_t* currConn = getConnectionByHandle(btlecIn, evt->data.evt_gatt_procedure_completed.connection);
			if( currConn != NULL )
			{
				cxa_logger_debug(&btlecIn->super.logger, "procedure complete");
				cxa_siLabsBgApi_btle_connection_handleEvent_procedureComplete(currConn, evt->data.evt_gatt_procedure_completed.result);
				retVal = true;
			}
			break;
		}

		case gecko_evt_gatt_service_id:
		{
			cxa_siLabsBgApi_btle_connection_t* currConn = getConnectionByHandle(btlecIn, evt->data.evt_gatt_service.connection);
			if( currConn != NULL )
			{
				cxa_logger_debug(&btlecIn->super.logger, "resolved service");
				cxa_btle_uuid_t tmpUuid;
				if( cxa_btle_uuid_init(&tmpUuid, evt->data.evt_gatt_service.uuid.data, evt->data.evt_gatt_service.uuid.len, true) )
				{
					cxa_siLabsBgApi_btle_connection_handleEvent_serviceResolved(currConn, &tmpUuid, evt->data.evt_gatt_service.service);
				}
				retVal = true;
			}
			break;
		}

		case gecko_evt_gatt_characteristic_id:
		{
			cxa_siLabsBgApi_btle_connection_t* currConn = getConnectionByHandle(btlecIn, evt->data.evt_gatt_characteristic.connection);
			if( currConn != NULL )
			{
				cxa_logger_debug(&btlecIn->super.logger, "resolved characteristic");
				cxa_btle_uuid_t tmpUuid;
				if( cxa_btle_uuid_init(&tmpUuid, evt->data.evt_gatt_characteristic.uuid.data, evt->data.evt_gatt_characteristic.uuid.len, true) )
				{
					cxa_siLabsBgApi_btle_connection_handleEvent_characteristicResolved(currConn, &tmpUuid, evt->data.evt_gatt_characteristic.characteristic);
				}
				retVal = true;
			}
			break;
		}

		case gecko_evt_gatt_characteristic_value_id:
		{
			cxa_siLabsBgApi_btle_connection_t* currConn = getConnectionByHandle(btlecIn, evt->data.evt_gatt_characteristic_value.connection);
			if( currConn != NULL )
			{
				cxa_logger_trace(&btlecIn->super.logger, "got characteristic value");
				cxa_siLabsBgApi_btle_connection_handleEvent_characteristicValueUpdated(currConn, evt->data.evt_gatt_characteristic_value.characteristic, evt->data.evt_gatt_characteristic_value.att_opcode, evt->data.evt_gatt_characteristic_value.value.data, evt->data.evt_gatt_characteristic_value.value.len);
				retVal = true;
			}
			break;
		}

		case gecko_evt_le_gap_scan_response_id:
		{
//			cxa_logger_debug_memDump(&btlecIn->super.logger, "rxBytes: ", evt->data.evt_le_gap_scan_response.data.data, evt->data.evt_le_gap_scan_response.data.len, NULL);

			cxa_btle_advPacket_t rxPacket;
			if( cxa_btle_advPacket_init(&rxPacket,
										evt->data.evt_le_gap_scan_response.address.addr,
										(evt->data.evt_le_gap_scan_response.address_type == le_gap_address_type_random),
										evt->data.evt_le_gap_scan_response.rssi,
										evt->data.evt_le_gap_scan_response.data.data,
										evt->data.evt_le_gap_scan_response.data.len) )
			{
				// if we made it here, we parsed the packet successfully...notify our listeners
				cxa_btle_central_notify_advertRx(&btlecIn->super, &rxPacket);
			}
			else
			{
				cxa_logger_warn(&btlecIn->super.logger, "malformed advert packet");
			}
			retVal = true;

			break;
		}

		default:
			cxa_logger_debug(&btlecIn->super.logger, "unhandled event: 0x%08X", BGLIB_MSG_ID(evt->header));
			break;
	}

	return retVal;
}


// ******** local function implementations ********
static cxa_siLabsBgApi_btle_connection_t* getUnusedConnection(cxa_siLabsBgApi_btle_central_t *const btlecIn)
{
	cxa_assert(btlecIn);

	cxa_siLabsBgApi_btle_connection_t* retVal = NULL;

	for( size_t i = 0; i < sizeof(btlecIn->conns)/sizeof(*btlecIn->conns); i++ )
	{
		if( !cxa_siLabsBgApi_btle_connection_isUsed(&btlecIn->conns[i]) )
		{
			retVal = &btlecIn->conns[i];
			break;
		}
	}

	return retVal;
}


static cxa_siLabsBgApi_btle_connection_t* getConnectionByAddress(cxa_siLabsBgApi_btle_central_t *const btlecIn, cxa_eui48_t *const targetAddrIn)
{
	cxa_assert(btlecIn);
	cxa_assert(targetAddrIn);

	cxa_siLabsBgApi_btle_connection_t* retVal = NULL;

	for( size_t i = 0; i < sizeof(btlecIn->conns)/sizeof(*btlecIn->conns); i++ )
	{
		if( cxa_eui48_isEqual(&btlecIn->conns[i].targetAddress, targetAddrIn) )
		{
			retVal = &btlecIn->conns[i];
			break;
		}
	}

	return retVal;
}


static cxa_siLabsBgApi_btle_connection_t* getConnectionByHandle(cxa_siLabsBgApi_btle_central_t *const btlecIn, uint8_t connHandleIn)
{
	cxa_assert(btlecIn);

	cxa_siLabsBgApi_btle_connection_t* retVal = NULL;

	for( size_t i = 0; i < sizeof(btlecIn->conns)/sizeof(*btlecIn->conns); i++ )
	{
		if( btlecIn->conns[i].connHandle == connHandleIn )
		{
			retVal = &btlecIn->conns[i];
			break;
		}
	}

	return retVal;
}


static cxa_btle_central_state_t scm_getState(cxa_btle_central_t *const superIn)
{
	cxa_siLabsBgApi_btle_central_t *const btlecIn = (cxa_siLabsBgApi_btle_central_t *const)superIn;
	cxa_assert(btlecIn);

	return cxa_siLabsBgApi_module_getState();
}


static void scm_startScan(cxa_btle_central_t *const superIn, bool isActiveIn)
{
	cxa_siLabsBgApi_btle_central_t *const btlecIn = (cxa_siLabsBgApi_btle_central_t *const)superIn;
	cxa_assert(btlecIn);
	cxa_assert_msg(!isActiveIn, "not yet implemented");

	cxa_runLoop_dispatchNextIteration(btlecIn->threadId, runLoopOneShot_startScan, (void*)btlecIn);
}


static void scm_stopScan(cxa_btle_central_t *const superIn)
{
	cxa_siLabsBgApi_btle_central_t *const btlecIn = (cxa_siLabsBgApi_btle_central_t *const)superIn;
	cxa_assert(btlecIn);

	cxa_runLoop_dispatchNextIteration(btlecIn->threadId, runLoopOneShot_stopScan, (void*)btlecIn);
}


static void scm_startConnection(cxa_btle_central_t *const superIn, cxa_eui48_t *const targetAddrIn, bool isRandomAddrIn)
{
	cxa_siLabsBgApi_btle_central_t *const btlecIn = (cxa_siLabsBgApi_btle_central_t *const)superIn;
	cxa_assert(btlecIn);

	cxa_siLabsBgApi_btle_connection_t* newConnection = getUnusedConnection(btlecIn);
	cxa_assert_msg((newConnection != NULL), "too many open connections");
	cxa_siLabsBgApi_btle_connection_startConnection(newConnection, targetAddrIn, isRandomAddrIn);
}


static void scm_stopConnection(cxa_btle_central_t *const superIn, cxa_eui48_t *const targetAddrIn)
{
	cxa_siLabsBgApi_btle_central_t *const btlecIn = (cxa_siLabsBgApi_btle_central_t *const)superIn;
	cxa_assert(btlecIn);

	cxa_siLabsBgApi_btle_connection_t* currConn = getConnectionByAddress(btlecIn, targetAddrIn);
	if( currConn != NULL )
	{
		cxa_siLabsBgApi_btle_connection_stopConnection(currConn);
	}
	else
	{
		cxa_eui48_string_t addr_str;
		cxa_eui48_toString(targetAddrIn, &addr_str);
		cxa_logger_warn(&btlecIn->super.logger, "not connected to '%s'", addr_str);
	}
}


static void scm_readFromCharacteristic(cxa_btle_central_t *const superIn,
									   cxa_eui48_t *const targetAddrIn,
									   const char *const serviceUuidIn,
									   const char *const characteristicUuidIn)
{
	cxa_siLabsBgApi_btle_central_t *const btlecIn = (cxa_siLabsBgApi_btle_central_t *const)superIn;
	cxa_assert(btlecIn);

	// make sure we know about this connection
	cxa_siLabsBgApi_btle_connection_t* currConn = getConnectionByAddress(btlecIn, targetAddrIn);
	if( currConn != NULL )
	{
		cxa_siLabsBgApi_btle_connection_readFromCharacteristic(currConn, serviceUuidIn, characteristicUuidIn);
	}
	else
	{
		cxa_eui48_string_t addr_str;
		cxa_eui48_toString(targetAddrIn, &addr_str);
		cxa_logger_warn(&btlecIn->super.logger, "not connected to '%s'", addr_str);
		cxa_btle_central_notify_readComplete(&btlecIn->super, targetAddrIn, serviceUuidIn, characteristicUuidIn, false, NULL);
	}
}


static void scm_writeToCharacteristic(cxa_btle_central_t *const superIn,
									  cxa_eui48_t *const targetAddrIn,
									  const char *const serviceUuidIn,
									  const char *const characteristicUuidIn,
									  cxa_fixedByteBuffer_t *const dataIn)
{
	cxa_siLabsBgApi_btle_central_t *const btlecIn = (cxa_siLabsBgApi_btle_central_t *const)superIn;
	cxa_assert(btlecIn);

	// make sure we know about this connection
	cxa_siLabsBgApi_btle_connection_t* currConn = getConnectionByAddress(btlecIn, targetAddrIn);
	if( currConn != NULL )
	{
		cxa_siLabsBgApi_btle_connection_writeToCharacteristic(currConn, serviceUuidIn, characteristicUuidIn, dataIn);
	}
	else
	{
		cxa_eui48_string_t addr_str;
		cxa_eui48_toString(targetAddrIn, &addr_str);
		cxa_logger_warn(&btlecIn->super.logger, "not connected to '%s'", addr_str);
		cxa_btle_central_notify_writeComplete(&btlecIn->super, targetAddrIn, serviceUuidIn, characteristicUuidIn, false);
	}
}


static void scm_changeNotifications(cxa_btle_central_t *const superIn,
		  	  	  	  	  	  	    cxa_eui48_t *const targetAddrIn,
									const char *const serviceUuidIn,
									const char *const characteristicUuidIn,
									bool enableNotifications)
{
	cxa_siLabsBgApi_btle_central_t *const btlecIn = (cxa_siLabsBgApi_btle_central_t *const)superIn;
	cxa_assert(btlecIn);

	// make sure we know about this connection
	cxa_siLabsBgApi_btle_connection_t* currConn = getConnectionByAddress(btlecIn, targetAddrIn);
	if( currConn != NULL )
	{
		cxa_siLabsBgApi_btle_connection_changeNotifications(currConn, serviceUuidIn, characteristicUuidIn, enableNotifications);
	}
	else
	{
		cxa_eui48_string_t addr_str;
		cxa_eui48_toString(targetAddrIn, &addr_str);
		cxa_logger_warn(&btlecIn->super.logger, "not connected to '%s'", addr_str);
		cxa_btle_central_notify_writeComplete(&btlecIn->super, targetAddrIn, serviceUuidIn, characteristicUuidIn, false);
	}
}


static void runLoopOneShot_startScan(void* userVarIn)
{
	cxa_siLabsBgApi_btle_central_t *const btlecIn = (cxa_siLabsBgApi_btle_central_t *const)userVarIn;
	cxa_assert(btlecIn);

	struct gecko_msg_le_gap_start_discovery_rsp_t* rsp = gecko_cmd_le_gap_start_discovery(le_gap_phy_1m, le_gap_discover_generic);
	if( rsp->result != 0 )
	{
		cxa_logger_warn(&btlecIn->super.logger, "scan start failed");
		cxa_btle_central_notify_scanStart(&btlecIn->super, false);
		return;
	}
	// if we made it here, scan start was successful

	cxa_btle_central_notify_scanStart(&btlecIn->super, true);
}


static void runLoopOneShot_stopScan(void* userVarIn)
{
	cxa_siLabsBgApi_btle_central_t *const btlecIn = (cxa_siLabsBgApi_btle_central_t *const)userVarIn;
	cxa_assert(btlecIn);

	gecko_cmd_le_gap_end_procedure();
}
