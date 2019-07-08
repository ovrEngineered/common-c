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

static void runLoopOneShot_startScan(void* userVarIn);
static void runLoopOneShot_stopScan(void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_siLabsBgApi_btle_central_init(cxa_siLabsBgApi_btle_central_t *const btlecIn, int threadIdIn)
{
	cxa_assert(btlecIn);

	// save our references and setup our internal state
	btlecIn->threadId = threadIdIn;
	btlecIn->isConnectionInProgress = false;

	// initialize our connections
	for( size_t i = 0; i < sizeof(btlecIn->conns)/sizeof(*btlecIn->conns); i++ )
	{
		cxa_siLabsBgApi_btle_connection_init(&btlecIn->conns[i], &btlecIn->super, threadIdIn);
	}

	// initialize our super class
	cxa_btle_central_init(&btlecIn->super, scm_getState, scm_startScan, scm_stopScan, scm_startConnection);
}


bool cxa_siLabsBgApi_btle_central_setConnectionInterval(cxa_siLabsBgApi_btle_central_t *const btlecIn, cxa_eui48_t *const targetConnectionAddressIn, uint16_t connectionInterval_msIn)
{
	cxa_assert(btlecIn);
	cxa_assert(targetConnectionAddressIn);

	cxa_siLabsBgApi_btle_connection_t* targetConn = getConnectionByAddress(btlecIn, targetConnectionAddressIn);
	if( targetConn == NULL ) return false;

	// validate per Apple Accessory Design Guidlines page 70
	// https://developer.apple.com/accessories/Accessory-Design-Guidelines.pdf


	// Interval Min ≥ 15 ms
	if( connectionInterval_msIn < 15 ) return false;

	// Interval Min modulo 15 ms == 0
	if( (connectionInterval_msIn % 15) != 0 ) return false;
	uint16_t min_interval_ms = connectionInterval_msIn;


	// One of the following:
	//  - Interval Min + 15 ms ≤ Interval Max
	//  - Interval Min == Interval Max == 15 ms
	uint16_t max_interval_ms = (min_interval_ms == 15) ? min_interval_ms : (min_interval_ms + 15);


	// Interval Max * (Slave Latency + 1) ≤ 2 seconds
	uint16_t latency = (2.0 / ((float)(max_interval_ms * 1000.0))) - 1.0;


	// Interval Max * (Slave Latency + 1) * 3 < connSupervisionTimeout
	// minimum of 100
	uint16_t timeout_ms = (max_interval_ms * (latency + 1) * 3);
	if( timeout_ms < 100 ) timeout_ms = 100;


	cxa_logger_info(&btlecIn->super.logger, "setting conn. params   minInt: %d ms   maxIn: %dms  lat: %d  timeout: %d ms", min_interval_ms, max_interval_ms, latency, timeout_ms);
	struct gecko_msg_le_connection_set_parameters_rsp_t* resp = gecko_cmd_le_connection_set_parameters(targetConn->connHandle,
																									   (min_interval_ms / 1.25),
																									   (max_interval_ms / 1.25),
																									   latency,
																									   timeout_ms / 10);
	if( resp->result != 0 )
	{
		cxa_logger_warn(&btlecIn->super.logger, "error setting conn. params: 0x%04x", resp->result);
		return false;
	}

	// if we made it here, we were successful;
	return true;
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
			btlecIn->isConnectionInProgress = false;

			cxa_eui48_t connectedAddr;
			cxa_eui48_init(&connectedAddr, evt->data.evt_le_connection_opened.address.addr);

			cxa_eui48_string_t connectedAddrStr;
			cxa_eui48_toString(&connectedAddr, &connectedAddrStr);

			// notify our connection
			cxa_siLabsBgApi_btle_connection_t* currConn = getConnectionByHandle(btlecIn, evt->data.evt_le_connection_opened.connection);
			if( currConn != NULL )
			{
				cxa_logger_debug(&btlecIn->super.logger, "connected to '%s' handle %d", connectedAddrStr.str, evt->data.evt_le_connection_opened.connection);
				cxa_siLabsBgApi_btle_connection_handleEvent_opened(currConn);
				retVal = true;
			}
			break;
		}

		case gecko_evt_le_connection_closed_id:
		{
			btlecIn->isConnectionInProgress = false;

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
		cxa_siLabsBgApi_btle_connection_t* currConn = &btlecIn->conns[i];
		if( cxa_siLabsBgApi_btle_connection_isUsed(currConn) &&
			cxa_eui48_isEqual(&currConn->super.targetAddr, targetAddrIn) )
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
		cxa_siLabsBgApi_btle_connection_t* currConn = &btlecIn->conns[i];
		if( cxa_siLabsBgApi_btle_connection_isUsed(currConn) &&
			currConn->connHandle == connHandleIn )
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

	// can only have one outstanding call to the "connect" command (until failed or cancelled)
	if( btlecIn->isConnectionInProgress )
	{
		cxa_btle_central_notify_connectionStarted(&btlecIn->super, false, NULL);
		return;
	}
	// if we made it here, we don't have any connections in progress
	btlecIn->isConnectionInProgress = true;

	// start our connection
	cxa_siLabsBgApi_btle_connection_t* newConnection = getUnusedConnection(btlecIn);
	cxa_assert_msg((newConnection != NULL), "too many open connections");
	cxa_siLabsBgApi_btle_connection_startConnection(newConnection, targetAddrIn, isRandomAddrIn);
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

	cxa_btle_central_notify_scanStop(&btlecIn->super);
}
