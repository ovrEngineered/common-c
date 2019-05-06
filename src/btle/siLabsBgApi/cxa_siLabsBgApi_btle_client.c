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
#include "cxa_siLabsBgApi_btle_client.h"


// ******** includes ********
#include <gecko_bglib.h>

#include <cxa_array.h>
#include <cxa_assert.h>
#include <cxa_ioStream_peekable.h>
#include <cxa_runLoop.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_DEBUG
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
BGLIB_DEFINE();							// needs to be defined to use bglib functions

#define WAIT_BOOT_TIME_MS				4000


// ******** local type definitions ********
typedef enum
{
	RADIOSTATE_RESET,
	RADIOSTATE_WAIT_BOOT,
	RADIOSTATE_READY,
}radioState_t;


// ******** local function prototypes ********
static void appHandleEvents(cxa_siLabsBgApi_btle_client_t *const btlecIn, struct gecko_cmd_packet *evt);

static cxa_siLabsBgApi_btle_connection_t* getUnusedConnection(cxa_siLabsBgApi_btle_client_t *const btlecIn);
static cxa_siLabsBgApi_btle_connection_t* getConnectionByAddress(cxa_siLabsBgApi_btle_client_t *const btlecIn, cxa_eui48_t *const targetAddrIn);
static cxa_siLabsBgApi_btle_connection_t* getConnectionByHandle(cxa_siLabsBgApi_btle_client_t *const btlecIn, uint8_t connHandleIn);

static void stateCb_reset_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_waitForBoot_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_waitBoot_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_ready_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);

static void stateCb_xxx_state(cxa_stateMachine_t *const smIn, void *userVarIn);

static cxa_btle_client_state_t scm_getState(cxa_btle_client_t *const superIn);
static void scm_startScan(cxa_btle_client_t *const superIn, bool isActiveIn);
static void scm_stopScan(cxa_btle_client_t *const superIn);
static void scm_startConnection(cxa_btle_client_t *const superIn, cxa_eui48_t *const targetAddrIn, bool isRandomAddrIn);
static void scm_stopConnection(cxa_btle_client_t *const superIn, cxa_eui48_t *const targetAddrIn);
static void scm_readFromCharacteristic(cxa_btle_client_t *const superIn,
									   cxa_eui48_t *const targetAddrIn,
									   const char *const serviceUuidIn,
									   const char *const characteristicUuidIn);
static void scm_writeToCharacteristic(cxa_btle_client_t *const superIn,
									  cxa_eui48_t *const targetAddrIn,
									  const char *const serviceIdIn,
									  const char *const characteristicIdIn,
									  cxa_fixedByteBuffer_t *const dataIn);
static void scm_changeNotifications(cxa_btle_client_t *const superIn,
		  	  	  	  	  	  	    cxa_eui48_t *const targetAddrIn,
									const char *const serviceUuidIn,
									const char *const characteristicUuidIn,
									bool enableNotifications);

static void runLoopOneShot_startScan(void* userVarIn);
static void runLoopOneShot_stopScan(void* userVarIn);

static void bglib_cb_output(uint32_t numBytesIn, uint8_t* dataIn);
static int32_t bglib_cb_input(uint32_t numBytesToReadIn, uint8_t* dataOut);
static int32_t bglib_cb_peek(void);


// ********  local variable declarations *********
static cxa_siLabsBgApi_btle_client_t* SINGLETON = NULL;


// ******** global function implementations ********
void cxa_siLabsBgApi_btle_client_init(cxa_siLabsBgApi_btle_client_t *const btlecIn, cxa_ioStream_t *const ioStreamIn, int threadIdIn)
{
	cxa_assert_msg((SINGLETON == NULL), "can only have one instance");
	cxa_assert(btlecIn);
	cxa_assert(ioStreamIn);

	// save our references and setup our internal state
	SINGLETON = btlecIn;
	btlecIn->threadId = threadIdIn;
	cxa_ioStream_peekable_init(&btlecIn->ios_usart, ioStreamIn);
	cxa_logger_init(&btlecIn->logger, "bgApiBtleC");
	btlecIn->hasBootFailed = false;

	// initialize our connections
	for( size_t i = 0; i < sizeof(btlecIn->conns)/sizeof(*btlecIn->conns); i++ )
	{
		cxa_siLabsBgApi_btle_connection_init(&btlecIn->conns[i], &btlecIn->super, threadIdIn);
	}

	// setup our state machine
	cxa_stateMachine_init(&btlecIn->stateMachine, "bgApiBtleC", threadIdIn);
	cxa_stateMachine_addState(&btlecIn->stateMachine, RADIOSTATE_RESET, "reset", stateCb_reset_enter, NULL, NULL, (void*)btlecIn);
	cxa_stateMachine_addState_timed(&btlecIn->stateMachine, RADIOSTATE_WAIT_BOOT, "reset", RADIOSTATE_RESET, WAIT_BOOT_TIME_MS, stateCb_waitForBoot_enter, stateCb_xxx_state, stateCb_waitBoot_leave, (void*)btlecIn);
	cxa_stateMachine_addState(&btlecIn->stateMachine, RADIOSTATE_READY, "ready", stateCb_ready_enter, stateCb_xxx_state, NULL, (void*)btlecIn);
	cxa_stateMachine_setInitialState(&btlecIn->stateMachine, RADIOSTATE_RESET);

	// initialize our super class
	cxa_btle_client_init(&btlecIn->super, scm_getState, scm_startScan, scm_stopScan, scm_startConnection, scm_stopConnection, scm_readFromCharacteristic, scm_writeToCharacteristic, scm_changeNotifications);

	// setup our BGLib
	BGLIB_INITIALIZE_NONBLOCK(bglib_cb_output, bglib_cb_input, bglib_cb_peek);
}


// ******** local function implementations ********
static void appHandleEvents(cxa_siLabsBgApi_btle_client_t *const btlecIn, struct gecko_cmd_packet *evt)
{
	cxa_assert(btlecIn);
	if( NULL == evt ) return;

	radioState_t currConnState = cxa_stateMachine_getCurrentState(&btlecIn->stateMachine);

	// Handle events
	switch( BGLIB_MSG_ID(evt->header) )
	{
		case gecko_evt_system_boot_id:
		{
			if( currConnState == RADIOSTATE_WAIT_BOOT )
			{
				cxa_logger_debug(&btlecIn->logger, "radio booted");
			}
			else
			{
				cxa_logger_warn(&btlecIn->logger, "unexpected radio boot");
			}
			cxa_stateMachine_transition(&btlecIn->stateMachine, RADIOSTATE_READY);
			break;
		}

		case gecko_evt_le_connection_opened_id:
		{
			cxa_logger_debug(&btlecIn->logger, "connected");

			cxa_eui48_t connectedAddr;
			cxa_eui48_init(&connectedAddr, evt->data.evt_le_connection_opened.address.addr);
			// update our connection handle
			cxa_siLabsBgApi_btle_connection_t* currConn = getConnectionByAddress(btlecIn, &connectedAddr);
			if( currConn != NULL )
			{
				cxa_siLabsBgApi_btle_connection_handleEvent_opened(currConn, evt->data.evt_le_connection_opened.connection);
			}
			break;
		}

		case gecko_evt_le_connection_closed_id:
		{
			cxa_logger_debug(&btlecIn->logger, "disconnected");
			cxa_siLabsBgApi_btle_connection_t* currConn = getConnectionByHandle(btlecIn, evt->data.evt_le_connection_closed.connection);
			if( currConn != NULL )
			{
				cxa_siLabsBgApi_btle_connection_handleEvent_closed(currConn, evt->data.evt_le_connection_closed.reason);
			}
			break;
		}

		case gecko_evt_gatt_procedure_completed_id:
		{
			cxa_logger_debug(&btlecIn->logger, "procedure complete");
			cxa_siLabsBgApi_btle_connection_t* currConn = getConnectionByHandle(btlecIn, evt->data.evt_gatt_procedure_completed.connection);
			if( currConn != NULL )
			{
				cxa_siLabsBgApi_btle_connection_handleEvent_procedureComplete(currConn, evt->data.evt_gatt_procedure_completed.result);
			}
			break;
		}

		case gecko_evt_gatt_service_id:
		{
			cxa_logger_debug(&btlecIn->logger, "resolved service");
			cxa_siLabsBgApi_btle_connection_t* currConn = getConnectionByHandle(btlecIn, evt->data.evt_gatt_service.connection);
			if( currConn != NULL )
			{
				cxa_btle_uuid_t tmpUuid;
				if( cxa_btle_uuid_init(&tmpUuid, evt->data.evt_gatt_service.uuid.data, evt->data.evt_gatt_service.uuid.len, true) )
				{
					cxa_siLabsBgApi_btle_connection_handleEvent_serviceResolved(currConn, &tmpUuid, evt->data.evt_gatt_service.service);
				}

			}
			break;
		}

		case gecko_evt_gatt_characteristic_id:
		{
			cxa_logger_debug(&btlecIn->logger, "resolved characteristic");
			cxa_siLabsBgApi_btle_connection_t* currConn = getConnectionByHandle(btlecIn, evt->data.evt_gatt_characteristic.connection);
			if( currConn != NULL )
			{
				cxa_btle_uuid_t tmpUuid;
				if( cxa_btle_uuid_init(&tmpUuid, evt->data.evt_gatt_characteristic.uuid.data, evt->data.evt_gatt_characteristic.uuid.len, true) )
				{
					cxa_siLabsBgApi_btle_connection_handleEvent_characteristicResolved(currConn, &tmpUuid, evt->data.evt_gatt_characteristic.characteristic);
				}

			}
			break;
		}

		case gecko_evt_gatt_characteristic_value_id:
		{
			cxa_logger_debug(&btlecIn->logger, "got characteristic value");
			cxa_siLabsBgApi_btle_connection_t* currConn = getConnectionByHandle(btlecIn, evt->data.evt_gatt_characteristic_value.connection);
			if( currConn != NULL )
			{
				cxa_siLabsBgApi_btle_connection_handleEvent_characteristicValueUpdated(currConn, evt->data.evt_gatt_characteristic_value.characteristic, evt->data.evt_gatt_characteristic_value.att_opcode, evt->data.evt_gatt_characteristic_value.value.data, evt->data.evt_gatt_characteristic_value.value.len);
			}
			break;
		}

		case gecko_evt_le_gap_scan_response_id:
		{
//			cxa_logger_debug_memDump(&btlecIn->logger, "rxBytes: ", evt->data.evt_le_gap_scan_response.data.data, evt->data.evt_le_gap_scan_response.data.len, NULL);

			cxa_btle_advPacket_t rxPacket;
			if( cxa_btle_advPacket_init(&rxPacket,
										evt->data.evt_le_gap_scan_response.address.addr,
										(evt->data.evt_le_gap_scan_response.address_type == le_gap_address_type_random),
										evt->data.evt_le_gap_scan_response.rssi,
										evt->data.evt_le_gap_scan_response.data.data,
										evt->data.evt_le_gap_scan_response.data.len) )
			{
				// if we made it here, we parsed the packet successfully...notify our listeners
				cxa_btle_client_notify_advertRx(&btlecIn->super, &rxPacket);
			}
			else
			{
				cxa_logger_warn(&btlecIn->logger, "malformed advert packet");
			}

			break;
		}

		case gecko_evt_le_connection_parameters_id:
		case gecko_evt_le_connection_phy_status_id:
		case gecko_evt_gatt_mtu_exchanged_id:
			break;

		default:
			cxa_logger_debug(&btlecIn->logger, "unhandled event: 0x%08X", BGLIB_MSG_ID(evt->header));
			break;
	}
}


static cxa_siLabsBgApi_btle_connection_t* getUnusedConnection(cxa_siLabsBgApi_btle_client_t *const btlecIn)
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


static cxa_siLabsBgApi_btle_connection_t* getConnectionByAddress(cxa_siLabsBgApi_btle_client_t *const btlecIn, cxa_eui48_t *const targetAddrIn)
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


static cxa_siLabsBgApi_btle_connection_t* getConnectionByHandle(cxa_siLabsBgApi_btle_client_t *const btlecIn, uint8_t connHandleIn)
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


static void stateCb_reset_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_client_t *const btlecIn = (cxa_siLabsBgApi_btle_client_t *const)userVarIn;
	cxa_assert(btlecIn);

	cxa_logger_debug(&btlecIn->logger, "resetting radio");

	// tell the radio to reboot
	gecko_cmd_system_reset(0);

	// start waiting for the boot event
	cxa_stateMachine_transition(&btlecIn->stateMachine, RADIOSTATE_WAIT_BOOT);
}


static void stateCb_waitForBoot_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_client_t *const btlecIn = (cxa_siLabsBgApi_btle_client_t *const)userVarIn;
	cxa_assert(btlecIn);

	cxa_logger_debug(&btlecIn->logger, "waiting for radio boot");
}


static void stateCb_waitBoot_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_client_t *const btlecIn = (cxa_siLabsBgApi_btle_client_t *const)userVarIn;
	cxa_assert(btlecIn);

	// failed to receive our boot message
	if( nextStateIdIn == RADIOSTATE_RESET )
	{
		btlecIn->hasBootFailed = true;
		cxa_btle_client_notify_onFailedInit(&btlecIn->super, true);
	}
	else
	{
		// booted successfully
		btlecIn->hasBootFailed = false;
	}
}


static void stateCb_ready_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_client_t *const btlecIn = (cxa_siLabsBgApi_btle_client_t *const)userVarIn;
	cxa_assert(btlecIn);

	cxa_logger_info(&btlecIn->logger, "radio is ready");
	cxa_btle_client_notify_onBecomesReady(&btlecIn->super);
}


static void stateCb_xxx_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_client_t *const btlecIn = (cxa_siLabsBgApi_btle_client_t *const)userVarIn;
	cxa_assert(btlecIn);

	struct gecko_cmd_packet *evt = gecko_peek_event();
	appHandleEvents(btlecIn, evt);
}


static cxa_btle_client_state_t scm_getState(cxa_btle_client_t *const superIn)
{
	cxa_siLabsBgApi_btle_client_t *const btlecIn = (cxa_siLabsBgApi_btle_client_t *const)superIn;
	cxa_assert(btlecIn);

	cxa_btle_client_state_t retVal = CXA_BTLE_CLIENT_STATE_STARTUP;
	switch( cxa_stateMachine_getCurrentState(&btlecIn->stateMachine) )
	{
		case RADIOSTATE_RESET:
		case RADIOSTATE_WAIT_BOOT:
			retVal = btlecIn->hasBootFailed ? CXA_BTLE_CLIENT_STATE_STARTUPFAILED : CXA_BTLE_CLIENT_STATE_STARTUP;
			break;

		case RADIOSTATE_READY:
			retVal = CXA_BTLE_CLIENT_STATE_READY;
			break;
	}
	return retVal;
}


static void scm_startScan(cxa_btle_client_t *const superIn, bool isActiveIn)
{
	cxa_siLabsBgApi_btle_client_t *const btlecIn = (cxa_siLabsBgApi_btle_client_t *const)superIn;
	cxa_assert(btlecIn);
	cxa_assert_msg(!isActiveIn, "not yet implemented");

	cxa_runLoop_dispatchNextIteration(btlecIn->threadId, runLoopOneShot_startScan, (void*)btlecIn);
}


static void scm_stopScan(cxa_btle_client_t *const superIn)
{
	cxa_siLabsBgApi_btle_client_t *const btlecIn = (cxa_siLabsBgApi_btle_client_t *const)superIn;
	cxa_assert(btlecIn);

	cxa_runLoop_dispatchNextIteration(btlecIn->threadId, runLoopOneShot_stopScan, (void*)btlecIn);
}


static void scm_startConnection(cxa_btle_client_t *const superIn, cxa_eui48_t *const targetAddrIn, bool isRandomAddrIn)
{
	cxa_siLabsBgApi_btle_client_t *const btlecIn = (cxa_siLabsBgApi_btle_client_t *const)superIn;
	cxa_assert(btlecIn);

	cxa_siLabsBgApi_btle_connection_t* newConnection = getUnusedConnection(btlecIn);
	cxa_assert_msg((newConnection != NULL), "too many open connections");
	cxa_siLabsBgApi_btle_connection_startConnection(newConnection, targetAddrIn, isRandomAddrIn);
}


static void scm_stopConnection(cxa_btle_client_t *const superIn, cxa_eui48_t *const targetAddrIn)
{
	cxa_siLabsBgApi_btle_client_t *const btlecIn = (cxa_siLabsBgApi_btle_client_t *const)superIn;
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
		cxa_logger_warn(&btlecIn->logger, "not connected to '%s'", addr_str);
	}
}


static void scm_readFromCharacteristic(cxa_btle_client_t *const superIn,
									   cxa_eui48_t *const targetAddrIn,
									   const char *const serviceUuidIn,
									   const char *const characteristicUuidIn)
{
	cxa_siLabsBgApi_btle_client_t *const btlecIn = (cxa_siLabsBgApi_btle_client_t *const)superIn;
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
		cxa_logger_warn(&btlecIn->logger, "not connected to '%s'", addr_str);
		cxa_btle_client_notify_readComplete(&btlecIn->super, targetAddrIn, serviceUuidIn, characteristicUuidIn, false, NULL);
	}
}


static void scm_writeToCharacteristic(cxa_btle_client_t *const superIn,
									  cxa_eui48_t *const targetAddrIn,
									  const char *const serviceUuidIn,
									  const char *const characteristicUuidIn,
									  cxa_fixedByteBuffer_t *const dataIn)
{
	cxa_siLabsBgApi_btle_client_t *const btlecIn = (cxa_siLabsBgApi_btle_client_t *const)superIn;
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
		cxa_logger_warn(&btlecIn->logger, "not connected to '%s'", addr_str);
		cxa_btle_client_notify_writeComplete(&btlecIn->super, targetAddrIn, serviceUuidIn, characteristicUuidIn, false);
	}
}


static void scm_changeNotifications(cxa_btle_client_t *const superIn,
		  	  	  	  	  	  	    cxa_eui48_t *const targetAddrIn,
									const char *const serviceUuidIn,
									const char *const characteristicUuidIn,
									bool enableNotifications)
{
	cxa_siLabsBgApi_btle_client_t *const btlecIn = (cxa_siLabsBgApi_btle_client_t *const)superIn;
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
		cxa_logger_warn(&btlecIn->logger, "not connected to '%s'", addr_str);
		cxa_btle_client_notify_writeComplete(&btlecIn->super, targetAddrIn, serviceUuidIn, characteristicUuidIn, false);
	}
}


static void runLoopOneShot_startScan(void* userVarIn)
{
	cxa_siLabsBgApi_btle_client_t *const btlecIn = (cxa_siLabsBgApi_btle_client_t *const)userVarIn;
	cxa_assert(btlecIn);

	struct gecko_msg_le_gap_start_discovery_rsp_t* rsp = gecko_cmd_le_gap_start_discovery(le_gap_phy_1m, le_gap_discover_generic);
	if( rsp->result != 0 )
	{
		cxa_logger_warn(&btlecIn->logger, "scan start failed");
		cxa_btle_client_notify_scanStart(&btlecIn->super, false);
		return;
	}
	// if we made it here, scan start was successful

	cxa_btle_client_notify_scanStart(&btlecIn->super, true);
}


static void runLoopOneShot_stopScan(void* userVarIn)
{
	cxa_siLabsBgApi_btle_client_t *const btlecIn = (cxa_siLabsBgApi_btle_client_t *const)userVarIn;
	cxa_assert(btlecIn);

	gecko_cmd_le_gap_end_procedure();
}


static void bglib_cb_output(uint32_t numBytesIn, uint8_t* dataIn)
{
	cxa_assert(SINGLETON);

	cxa_logger_trace_memDump(&SINGLETON->logger, "write to bgm121: ", dataIn, numBytesIn, NULL);
	cxa_ioStream_writeBytes(&SINGLETON->ios_usart.super, dataIn, numBytesIn);
}


static int32_t bglib_cb_input(uint32_t numBytesToReadIn, uint8_t* dataOut)
{
	cxa_assert(SINGLETON);

	uint8_t rxByte;

	cxa_logger_trace(&SINGLETON->logger, "waiting for %d bytes", numBytesToReadIn);
	for( uint32_t numBytesRead = 0; numBytesRead < numBytesToReadIn; numBytesRead++ )
	{
		while(1)
		{
			cxa_ioStream_readStatus_t readStat = cxa_ioStream_readByte(&SINGLETON->ios_usart.super, &rxByte);
			if( readStat == CXA_IOSTREAM_READSTAT_GOTDATA )
			{
				// got our byte...continue
				dataOut[numBytesRead] = rxByte;
				break;
			}
			else if( readStat == CXA_IOSTREAM_READSTAT_ERROR )
			{
				// return non-zero on failure
				cxa_logger_error(&SINGLETON->logger, "error during read");
				return 1;
			}
		}
	}
	cxa_logger_trace(&SINGLETON->logger, "read complete (%d bytes)", numBytesToReadIn);

	// return 0 on success
	return 0;
}


static int32_t bglib_cb_peek(void)
{
	cxa_assert(SINGLETON);

	return cxa_ioStream_peekable_hasBytesAvailable(&SINGLETON->ios_usart);
}
