/**
 * @copyright 2016 opencxa.org
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
#include "cxa_blueGiga_btle_client.h"


// ******** includes ********
#include <string.h>

#include <cxa_assert.h>
#include <cxa_console.h>
#include <cxa_delay.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_INFO
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define RESET_TIME_MS					5000
#define WAIT_BOOT_TIME_MS				4000

#define COMMAND_TIMEOUT_MS				1500

#define READWRITE_TIMEOUT_MS				2000

#define CONNECTION_INTERVAL_MIN_MS		50			// must be intervals of 1.25ms
#define CONNECTION_INTERVAL_MAX_MS		500			// must be intervals of 1.25ms
#define CONNECTION_LATENCY				10			// num missed conn intervals
// per BTLE spec
#define CONNECTION_TIMEOUT_MS			((1 + CONNECTION_LATENCY) * CONNECTION_INTERVAL_MAX_MS * 2)

#define SCANNING_ACTIVITY_TIMEOUT_MS		10000


// ******** local type definitions ********
typedef enum
{
	CONNSTATE_RESET,
	CONNSTATE_WAIT_BOOT,
	CONNSTATE_CONFIG_PERIPHS,
	CONNSTATE_READY,
	CONNSTATE_SCANNING,
	CONNSTATE_CONNECTING,
	CONNSTATE_CONNECTED
}connState_t;


typedef enum
{
	PROCSTATE_NOT_READY,
	PROCSTATE_IDLE,
	PROCSTATE_RESOLVE_SERVICE_HANDLE,
	PROCSTATE_RESOLVE_CHARACTERISTIC_HANDLE,
	PROCSTATE_READ_CHARACTERISTIC,
	PROCSTATE_WRITE_CHARACTERISTIC,
	PROCSTATE_ERROR
}procedureState_t;


// ******** local function prototypes ********
static cxa_blueGiga_msgType_t getMsgType(cxa_fixedByteBuffer_t *const fbbIn);
static cxa_blueGiga_classId_t getClassId(cxa_fixedByteBuffer_t *const fbbIn);
static cxa_blueGiga_methodId_t getMethod(cxa_fixedByteBuffer_t *const fbbIn);

static void handleBgEvent(cxa_blueGiga_btle_client_t *const btlecIn, cxa_fixedByteBuffer_t *const packetIn);
static void handleBgResponse(cxa_blueGiga_btle_client_t *const btlecIn, cxa_fixedByteBuffer_t *const packetIn);
static void handleBgTimeout(void* userVarIn);
static void handleReadWriteTimeout(void* userVarIn);

static void protoParseCb_onPacketRx(cxa_fixedByteBuffer_t *const packetIn, void *const userVarIn);

static void responseCb_setScanParams(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn);
static void responseCb_discover(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn);
static void responseCb_endProcedure(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn);
static void responseCb_connectDirect(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn);
static void responseCb_findByGroupType(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn);
static void responseCb_findInformation(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn);
static void responseCb_attributeWrite(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn);

static void gpioCb_onGpiosConfigured(cxa_blueGiga_btle_client_t* btlecIn, bool wasSuccessfulIn);

static cxa_btle_client_state_t scm_getState(cxa_btle_client_t *const superIn);
static void scm_startScan(cxa_btle_client_t *const superIn, bool isActiveIn);
static void scm_stopScan(cxa_btle_client_t *const superIn);
static void scm_startConnection(cxa_btle_client_t *const superIn, cxa_eui48_t *const addrIn, bool isRandomAddrIn);
static void scm_stopConnection(cxa_btle_client_t *const superIn);
static bool scm_isConnected(cxa_btle_client_t *const superIn);
static void scm_writeToCharacteristic(cxa_btle_client_t *const superIn,
									  const char *const serviceIdIn,
									  const char *const characteristicIdIn,
									  cxa_fixedByteBuffer_t *const dataIn);

static void stateCb_conn_reset_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_conn_reset_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_conn_waitBoot_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_conn_configPeriphs_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_conn_ready_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_conn_scanning_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_conn_scanning_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_conn_connecting_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_conn_connected_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_conn_connected_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);

static void stateCb_currProc_idle_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_currProc_resolveService_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_currProc_resolveCharacteristic_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_currProc_readCharacteristic_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_currProc_writeCharacteristic_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_currProc_error_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_blueGiga_btle_client_init(cxa_blueGiga_btle_client_t *const btlecIn, cxa_ioStream_t *const iosIn, cxa_gpio_t *const gpio_resetIn, int threadIdIn)
{
	cxa_assert(btlecIn);
	cxa_assert(iosIn);
	cxa_assert(gpio_resetIn);

	// save our references and initialize our internal state
	btlecIn->gpio_reset = gpio_resetIn;
	btlecIn->hasBootFailed = false;
	cxa_softWatchDog_init(&btlecIn->inFlightRequest.watchdog, COMMAND_TIMEOUT_MS, threadIdIn, handleBgTimeout, (void*)btlecIn);
	cxa_softWatchDog_init(&btlecIn->currProcedure.watchdog, READWRITE_TIMEOUT_MS, threadIdIn, handleReadWriteTimeout, (void*)btlecIn);

	// initialize our superclass
	cxa_btle_client_init(&btlecIn->super, scm_getState,
						 scm_startScan, scm_stopScan,
						 scm_startConnection, scm_stopConnection, scm_isConnected, scm_writeToCharacteristic);

	// and our logger
	cxa_logger_init(&btlecIn->logger, "btleClient");

	// and our buffers
	cxa_fixedByteBuffer_initStd(&btlecIn->fbb_tx, btlecIn->fbb_tx_raw);
	cxa_fixedByteBuffer_initStd(&btlecIn->fbb_rx, btlecIn->fbb_rx_raw);

	// our protocol parser
	cxa_protocolParser_bgapi_init(&btlecIn->protoParse, iosIn, &btlecIn->fbb_rx, threadIdIn);
	cxa_protocolParser_addPacketListener(&btlecIn->protoParse.super, protoParseCb_onPacketRx, (void*)btlecIn);

	// our gpios (unused at the moment)
	for( size_t i = 0; i < (sizeof(btlecIn->gpios)/sizeof(*btlecIn->gpios)); i++ )
	{
		btlecIn->gpios[i].isUsed = false;
	}

	// our i2c
	cxa_blueGiga_i2cMaster_init(&btlecIn->i2cMaster, btlecIn);

	// and our connection state machine
	cxa_stateMachine_init(&btlecIn->stateMachine_conn, "blueGiga_conn", threadIdIn);
	cxa_stateMachine_addState_timed(&btlecIn->stateMachine_conn, CONNSTATE_RESET, "reset", CONNSTATE_WAIT_BOOT, RESET_TIME_MS, stateCb_conn_reset_enter, NULL, stateCb_conn_reset_leave, (void*)btlecIn);
	cxa_stateMachine_addState_timed(&btlecIn->stateMachine_conn, CONNSTATE_WAIT_BOOT, "waitBoot", CONNSTATE_RESET, WAIT_BOOT_TIME_MS, NULL, NULL, stateCb_conn_waitBoot_leave, (void*)btlecIn);
	cxa_stateMachine_addState(&btlecIn->stateMachine_conn, CONNSTATE_CONFIG_PERIPHS, "configPeriphs", stateCb_conn_configPeriphs_enter, NULL, NULL, (void*)btlecIn);
	cxa_stateMachine_addState(&btlecIn->stateMachine_conn, CONNSTATE_READY, "ready", stateCb_conn_ready_enter, NULL, NULL, (void*)btlecIn);
	cxa_stateMachine_addState(&btlecIn->stateMachine_conn, CONNSTATE_SCANNING, "scanning", stateCb_conn_scanning_enter, NULL, stateCb_conn_scanning_leave, (void*)btlecIn);
	cxa_stateMachine_addState(&btlecIn->stateMachine_conn, CONNSTATE_CONNECTING, "connecting", stateCb_conn_connecting_enter, NULL, NULL, (void*)btlecIn);
	cxa_stateMachine_addState(&btlecIn->stateMachine_conn, CONNSTATE_CONNECTED, "connected", stateCb_conn_connected_enter, NULL, stateCb_conn_connected_leave, (void*)btlecIn);
	cxa_stateMachine_setInitialState(&btlecIn->stateMachine_conn, CONNSTATE_RESET);

	// and our procedure state machine
	cxa_stateMachine_init(&btlecIn->stateMachine_currProcedure, "blueGiga_proc", threadIdIn);
	cxa_stateMachine_addState(&btlecIn->stateMachine_currProcedure, PROCSTATE_NOT_READY, "notReady", NULL, NULL, NULL, (void*)btlecIn);
	cxa_stateMachine_addState(&btlecIn->stateMachine_currProcedure, PROCSTATE_IDLE, "idle", stateCb_currProc_idle_enter, NULL, NULL, (void*)btlecIn);
	cxa_stateMachine_addState(&btlecIn->stateMachine_currProcedure, PROCSTATE_RESOLVE_SERVICE_HANDLE, "resServHan", stateCb_currProc_resolveService_enter, NULL, NULL, (void*)btlecIn);
	cxa_stateMachine_addState(&btlecIn->stateMachine_currProcedure, PROCSTATE_RESOLVE_CHARACTERISTIC_HANDLE, "resCharHan", stateCb_currProc_resolveCharacteristic_enter, NULL, NULL, (void*)btlecIn);
	cxa_stateMachine_addState(&btlecIn->stateMachine_currProcedure, PROCSTATE_READ_CHARACTERISTIC, "readChar", stateCb_currProc_readCharacteristic_enter, NULL, NULL, (void*)btlecIn);
	cxa_stateMachine_addState(&btlecIn->stateMachine_currProcedure, PROCSTATE_WRITE_CHARACTERISTIC, "writeChar", stateCb_currProc_writeCharacteristic_enter, NULL, NULL, (void*)btlecIn);
	cxa_stateMachine_addState(&btlecIn->stateMachine_currProcedure, PROCSTATE_ERROR, "error", stateCb_currProc_error_enter, NULL, NULL, (void*)btlecIn);
	cxa_stateMachine_setInitialState(&btlecIn->stateMachine_currProcedure, PROCSTATE_NOT_READY);
}


bool cxa_blueGiga_btle_client_sendCommand(cxa_blueGiga_btle_client_t *const btlecIn,
										  cxa_blueGiga_classId_t classIdIn, cxa_blueGiga_methodId_t methodIdIn, cxa_fixedByteBuffer_t *const payloadIn,
										  cxa_blueGiga_btle_client_cb_onResponse_t cb_onResponseIn, void* userVarIn)
{
	cxa_assert(btlecIn);

	// can only send one message at a time
	if( !cxa_softWatchDog_isPaused(&btlecIn->inFlightRequest.watchdog) )
	{
		return false;
	}

	// setup our packet
	cxa_fixedByteBuffer_clear(&btlecIn->fbb_tx);
	cxa_fixedByteBuffer_append_uint8(&btlecIn->fbb_tx, CXA_BLUEGIGA_MSGTYPE_COMMAND);
	cxa_fixedByteBuffer_append_uint8(&btlecIn->fbb_tx, (payloadIn != NULL) ? cxa_fixedByteBuffer_getSize_bytes(payloadIn) : 0);
	cxa_fixedByteBuffer_append_uint8(&btlecIn->fbb_tx, classIdIn);
	cxa_fixedByteBuffer_append_uint8(&btlecIn->fbb_tx, methodIdIn);
	if( payloadIn != NULL ) cxa_fixedByteBuffer_append_fbb(&btlecIn->fbb_tx, payloadIn);

	// mark our inflight request as in-use
	btlecIn->inFlightRequest.classId = classIdIn;
	btlecIn->inFlightRequest.methodId = methodIdIn;
	btlecIn->inFlightRequest.cb_onResponse = cb_onResponseIn;
	btlecIn->inFlightRequest.userVar = userVarIn;

	bool retVal = cxa_protocolParser_writePacket(&btlecIn->protoParse.super, &btlecIn->fbb_tx);
	if( retVal ) cxa_softWatchDog_kick(&btlecIn->inFlightRequest.watchdog);

	return retVal;
}


cxa_gpio_t* cxa_blueGiga_btle_client_getGpio(cxa_blueGiga_btle_client_t *const btlecIn, uint8_t portNumIn, uint8_t chanNumIn)
{
	cxa_assert(btlecIn);
	cxa_assert(portNumIn <= 2);
	cxa_assert(chanNumIn <= 7);

	cxa_gpio_t* retVal = NULL;
	for( size_t i = 0; i < (sizeof(btlecIn->gpios)/sizeof(*btlecIn->gpios)); i++ )
	{
		cxa_blueGiga_gpio_t* currGpio = &btlecIn->gpios[i];
		if( !currGpio->isUsed )
		{
			// if we made it here, we found an used gpio
			cxa_blueGiga_gpio_init(currGpio, btlecIn, portNumIn, chanNumIn);
			retVal = &currGpio->super;
			break;
		}
	}

	return retVal;
}


cxa_i2cMaster_t* cxa_blueGiga_btle_client_getI2cMaster(cxa_blueGiga_btle_client_t *const btlecIn)
{
	cxa_assert(btlecIn);

	return &btlecIn->i2cMaster.super;
}


// ******** local function implementations ********
static cxa_blueGiga_msgType_t getMsgType(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);

	uint8_t type_raw;
	if( !cxa_fixedByteBuffer_get_uint8(fbbIn, 0, type_raw) ) return CXA_BLUEGIGA_MSGTYPE_UNKNOWN;

	return (cxa_blueGiga_msgType_t)(type_raw & 0x80);
}


static cxa_blueGiga_classId_t getClassId(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);

	uint8_t classId_raw;
	if( !cxa_fixedByteBuffer_get_uint8(fbbIn, 2, classId_raw) ) return CXA_BLUEGIGA_CLASSID_UNKNOWN;

	return (cxa_blueGiga_classId_t)classId_raw;
}


static cxa_blueGiga_methodId_t getMethod(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);

	uint8_t commandId_raw;
	if( !cxa_fixedByteBuffer_get_uint8(fbbIn, 3, commandId_raw) ) return 0xFF;

	return (cxa_blueGiga_methodId_t)commandId_raw;
}


static void handleBgEvent(cxa_blueGiga_btle_client_t *const btlecIn, cxa_fixedByteBuffer_t *const packetIn)
{
	cxa_assert(btlecIn);
	cxa_assert(packetIn);

	cxa_blueGiga_classId_t classId = getClassId(packetIn);
	cxa_blueGiga_methodId_t method = getMethod(packetIn);

	cxa_logger_trace(&btlecIn->logger, "response  cid:%d  mtd:%d  %d bytes", classId, method, cxa_fixedByteBuffer_getSize_bytes(packetIn));

	if( (classId == CXA_BLUEGIGA_CLASSID_SYSTEM) && (method == CXA_BLUEGIGA_EVENTID_SYSTEM_BOOT) )
	{
		uint16_t sw_major, sw_minor, patch, build, linkLayer;
		uint8_t protocol, hw;
		if( !cxa_fixedByteBuffer_get_uint16LE(packetIn, 4, sw_major) ||
			!cxa_fixedByteBuffer_get_uint16LE(packetIn, 6, sw_minor) ||
			!cxa_fixedByteBuffer_get_uint16LE(packetIn, 8, patch) ||
			!cxa_fixedByteBuffer_get_uint16LE(packetIn, 10, build) ||
			!cxa_fixedByteBuffer_get_uint16LE(packetIn, 12, linkLayer) ||
			!cxa_fixedByteBuffer_get_uint8(packetIn, 14, protocol) ||
			!cxa_fixedByteBuffer_get_uint8(packetIn, 15, hw) ) return;

		cxa_logger_info(&btlecIn->logger, "boot  sw: %d.%d.%d  prot: %d  hw: %d",
				sw_major, sw_minor,patch, protocol, hw);

		cxa_stateMachine_transition(&btlecIn->stateMachine_conn, CONNSTATE_CONFIG_PERIPHS);
	}
	else if( (classId == CXA_BLUEGIGA_CLASSID_GAP) && (method == CXA_BLUEGIGA_EVENTID_GAP_SCANRESPONSE) )
	{
		// advertisement / scan response
		cxa_btle_advPacket_t rxPacket;
		uint8_t rssi_raw;
		if( !cxa_fixedByteBuffer_get_uint8(packetIn, 4, rssi_raw) ) return;
		rxPacket.rssi = (int8_t)rssi_raw;

		// make sure it's an advertisement
		uint8_t packetType_raw;
		if( !cxa_fixedByteBuffer_get_uint8(packetIn, 5, packetType_raw) ) return;
		cxa_blueGiga_scanResponsePacketType_t packetType = (cxa_blueGiga_scanResponsePacketType_t)packetType_raw;
		if( (packetType != CXA_BLUEGIGA_SCANRESP_TYPE_CONN_ADVERT) &&
			(packetType != CXA_BLUEGIGA_SCANRESP_TYPE_NONCONN_ADVERT) ) return;

		// address information
		if( !cxa_eui48_initFromBuffer(&rxPacket.addr, packetIn, 6) ) return;
		if( !cxa_fixedByteBuffer_get_uint8(packetIn, 12, rxPacket.isRandomAddress) ) return;

		// get our actual data bytes
		uint8_t* advBytes = cxa_fixedByteBuffer_get_pointerToIndex(packetIn, 15);
		if( advBytes == NULL) return;
		size_t numAdvertBytes = cxa_fixedByteBuffer_getSize_bytes(packetIn) - 15;

		// now the actual advert fields..first we must count them
		size_t numAdvFields = 0;
		if( !cxa_btle_client_countAdvFields(advBytes, numAdvertBytes, &numAdvFields) )
		{
			cxa_logger_warn(&btlecIn->logger, "malformed field, discarding");
			return;
		}

		// now that we've counted them, we can parse them pretty safely (index-wise)
		cxa_btle_advField_t fields_raw[numAdvFields];
		cxa_array_initStd(&rxPacket.advFields, fields_raw);
		if( !cxa_btle_client_parseAdvFieldsForPacket(&rxPacket, numAdvFields,
				advBytes, numAdvertBytes) )
		{
			cxa_logger_warn(&btlecIn->logger, "malformed field, discarding");
						return;
		}

		cxa_eui48_string_t addr_str;
		cxa_eui48_toString(&rxPacket.addr, &addr_str);
		cxa_logger_trace(&btlecIn->logger, "adv from %s(%s)  %ddBm  %d fields  %d bytes",
						addr_str.str,
						rxPacket.isRandomAddress ? "r" : "p",
						rxPacket.rssi,
						cxa_array_getSize_elems(&rxPacket.advFields),
						cxa_fixedByteBuffer_getSize_bytes(packetIn));

		// notify our listeners
		btlecIn->super.hasActivityAvailable = true;
		cxa_btle_client_notify_advertRx(&btlecIn->super, &rxPacket);
	}
	else if( (classId == CXA_BLUEGIGA_CLASSID_SYSTEM) && (method == CXA_BLUEGIGA_EVENTID_SYSTEM_PROTOCOLERROR) )
	{
		uint16_t reason;
		if( !cxa_fixedByteBuffer_get_uint16LE(packetIn, 4, reason) ) return;

		cxa_logger_warn(&btlecIn->logger, "protocol error: 0x%04X", reason);

		// if we have an outstanding request let the callback know it failed
		if( cxa_softWatchDog_isPaused(&btlecIn->inFlightRequest.watchdog) )
		{
			cxa_blueGiga_btle_client_cb_onResponse_t cb = btlecIn->inFlightRequest.cb_onResponse;
			cxa_softWatchDog_pause(&btlecIn->inFlightRequest.watchdog);
			if( cb != NULL ) cb(btlecIn, false, NULL, btlecIn->inFlightRequest.userVar);
		}

		cxa_stateMachine_transition(&btlecIn->stateMachine_conn, CONNSTATE_READY);
	}
	else if( (classId == CXA_BLUEGIGA_CLASSID_CONNECTION) && (method == CXA_BLUEGIGA_EVENTID_CONNECTION_STATUS) )
	{
		// there's a lot of info here, but we're mostly interested in the handle and connection flags
		uint8_t connHandle, connFlags;
		if( !cxa_fixedByteBuffer_get_uint8(packetIn, 4, connHandle) ||
			!cxa_fixedByteBuffer_get_uint8(packetIn, 5, connFlags) ) return;

		if( connHandle != btlecIn->currConnHandle )
		{
			cxa_logger_warn(&btlecIn->logger, "rx status for unknown handle %d", connHandle);
			return;
		}

		// if we made it here, the data is for our connection...make sure we're connected
		if( !(connFlags & (1 << 0)) )
		{
			cxa_logger_warn(&btlecIn->logger, "rx status reports not connected");
			return;
		}

		// if we made it here, we're connected...
		cxa_stateMachine_transition(&btlecIn->stateMachine_conn, CONNSTATE_CONNECTED);
	}
	else if( (classId == CXA_BLUEGIGA_CLASSID_CONNECTION) && (method == CXA_BLUEGIGA_EVENTID_CONNECTION_DISCONNECTED) )
	{
		cxa_logger_debug(&btlecIn->logger, "disconnect reported");
		if( cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_conn) == CONNSTATE_CONNECTED )
		{
			cxa_stateMachine_transition(&btlecIn->stateMachine_conn, CONNSTATE_READY);
		}
	}
	else if( (classId == CXA_BLUEGIGA_CLASSID_ATTR_CLIENT) && (method == CXA_BLUEGIGA_EVENTID_ATTR_CLIENT_PROCEDURE_COMPLETE) )
	{
		uint16_t result;
		if( !cxa_fixedByteBuffer_get_uint16LE(packetIn, 5, result) ) return;

		if( result > 0 )
		{
			cxa_logger_debug(&btlecIn->logger, "procedure completed with error: 0x%04X", result);
			cxa_stateMachine_transition(&btlecIn->stateMachine_currProcedure, PROCSTATE_ERROR);
			return;
		}

		// if we made it here, the procedure was successful...what we do next depends on what we were doing
		procedureState_t currProcState = cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_currProcedure);
		if( currProcState == PROCSTATE_RESOLVE_SERVICE_HANDLE )
		{
			if( (btlecIn->currProcedure.serviceHandle_start == 0) && (btlecIn->currProcedure.serviceHandle_end == 0) )
			{
				cxa_logger_warn(&btlecIn->logger, "failed to resolve service uuid");
				cxa_stateMachine_transition(&btlecIn->stateMachine_currProcedure, PROCSTATE_ERROR);
				return;
			}

			// if we made it here, we resolved the service uuid properly...try to resolve the characteristic id
			cxa_logger_debug(&btlecIn->logger, "service handle resolved");
			cxa_stateMachine_transition(&btlecIn->stateMachine_currProcedure, PROCSTATE_RESOLVE_CHARACTERISTIC_HANDLE);
		}
		else if( currProcState == PROCSTATE_RESOLVE_CHARACTERISTIC_HANDLE )
		{
			if( btlecIn->currProcedure.characteristicHandle == 0 )
			{
				cxa_logger_warn(&btlecIn->logger, "failed to resolve characteristic uuid");
				cxa_stateMachine_transition(&btlecIn->stateMachine_currProcedure, PROCSTATE_ERROR);
				return;
			}

			cxa_logger_debug(&btlecIn->logger, "characteristic handle resolved");
			if( btlecIn->currProcedure.procedureType == CXA_BLUEGIGA_BTLE_PROCEDURE_TYPE_READ )
			{
				cxa_stateMachine_transition(&btlecIn->stateMachine_currProcedure, PROCSTATE_READ_CHARACTERISTIC);
			}
			else if( btlecIn->currProcedure.procedureType == CXA_BLUEGIGA_BTLE_PROCEDURE_TYPE_WRITE )
			{
				cxa_stateMachine_transition(&btlecIn->stateMachine_currProcedure, PROCSTATE_WRITE_CHARACTERISTIC);
			}
		}
	}
	else if( (classId == CXA_BLUEGIGA_CLASSID_ATTR_CLIENT) && (method == CXA_BLUEGIGA_EVENTID_ATTR_CLIENT_GROUP_FOUND) )
	{
		if( cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_currProcedure) != PROCSTATE_RESOLVE_SERVICE_HANDLE ) return;

		uint16_t startHandle;
		if( !cxa_fixedByteBuffer_get_uint16LE(packetIn, 5, startHandle) ) return;

		uint16_t endHandle;
		if( !cxa_fixedByteBuffer_get_uint16LE(packetIn, 7, endHandle) ) return;

		uint8_t uuidSize_bytes;
		if( !cxa_fixedByteBuffer_get_uint8(packetIn, 9, uuidSize_bytes) ) return;

		cxa_btle_uuid_t uuid;
		cxa_btle_uuid_string_t uuid_str;
		if( !cxa_btle_uuid_initFromBuffer(&uuid, packetIn, 10, uuidSize_bytes) ) return;
		cxa_btle_uuid_toString(&uuid, &uuid_str);

		// if this isn't the UUID we're looking for, ignore it
		if( !cxa_btle_uuid_isEqual(&uuid, &btlecIn->currProcedure.readWriteTargetUuid_service) ) return;

		// if we made it here, we found our target service handle start/stop...record it
		// BUT we still need to wait for the end-of-procedure event to continue
		btlecIn->currProcedure.serviceHandle_start = startHandle;
		btlecIn->currProcedure.serviceHandle_end = endHandle;
	}
	else if( (classId = CXA_BLUEGIGA_CLASSID_ATTR_CLIENT) && (method == CXA_BLUEGIGA_EVENTID_ATTR_CLIENT_FIND_INFO_FOUND) )
	{
		uint16_t charHandle;
		if( !cxa_fixedByteBuffer_get_uint16LE(packetIn, 5, charHandle) ) return;

		uint8_t uuidSize_bytes;
		if( !cxa_fixedByteBuffer_get_uint8(packetIn, 7, uuidSize_bytes) ) return;

		cxa_btle_uuid_t uuid;
		cxa_btle_uuid_string_t uuid_str;
		if( !cxa_btle_uuid_initFromBuffer(&uuid, packetIn, 8, uuidSize_bytes) ) return;
		cxa_btle_uuid_toString(&uuid, &uuid_str);

		// if this isn't the UUID we're looking for, ignore it
		if( !cxa_btle_uuid_isEqual(&uuid, &btlecIn->currProcedure.readWriteTargetUuid_characteristic) ) return;

		// if we made it here, we found our target characteristic handle
		btlecIn->currProcedure.characteristicHandle = charHandle;
	}
	else
	{
		cxa_logger_debug(&btlecIn->logger, "unhandled event  cid:%d  mtd:%d  %d bytes", classId, method, cxa_fixedByteBuffer_getSize_bytes(packetIn));
	}
}


static void handleBgResponse(cxa_blueGiga_btle_client_t *const btlecIn, cxa_fixedByteBuffer_t *const packetIn)
{
	cxa_assert(btlecIn);
	cxa_assert(packetIn);

	cxa_blueGiga_classId_t classId = getClassId(packetIn);
	cxa_blueGiga_methodId_t method = getMethod(packetIn);

	// make sure we're actually waiting for a response
	if( cxa_softWatchDog_isPaused(&btlecIn->inFlightRequest.watchdog) )
	{
		cxa_logger_warn(&btlecIn->logger, "unexpected response  cid:%d  mtd:%d  %d bytes", classId, method, cxa_fixedByteBuffer_getSize_bytes(packetIn));
		return;
	}

	// if we made it here, we were waiting for a response
	cxa_logger_trace(&btlecIn->logger, "response  cid:%d  mtd:%d  %d bytes", classId, method, cxa_fixedByteBuffer_getSize_bytes(packetIn));

	// make sure it's the response we were looking for....
	if( (btlecIn->inFlightRequest.classId != classId) || (btlecIn->inFlightRequest.methodId != method) )
	{
		cxa_logger_warn(&btlecIn->logger, "mismatched response (exp/act)  cid:%d/%d  mtd:%d/%d",
						 btlecIn->inFlightRequest.classId, classId,
						 btlecIn->inFlightRequest.methodId, method);

		// reset paused _before_ we call our callback (so the callback can send a message if it desires)
		cxa_blueGiga_btle_client_cb_onResponse_t cb = btlecIn->inFlightRequest.cb_onResponse;
		cxa_softWatchDog_pause(&btlecIn->inFlightRequest.watchdog);
		if( cb != NULL ) cb(btlecIn, false, NULL, btlecIn->inFlightRequest.userVar);
		return;
	}

	// this _was_ the response we were looking for...call our callback
	cxa_fixedByteBuffer_t fbb_payload;
	cxa_fixedByteBuffer_init_subBufferRemainingElems(&fbb_payload, packetIn, 4);
	// reset paused _before_ we call our callback (so the callback can send a message if it desires)
	cxa_blueGiga_btle_client_cb_onResponse_t cb = btlecIn->inFlightRequest.cb_onResponse;
	cxa_softWatchDog_pause(&btlecIn->inFlightRequest.watchdog);
	if( cb != NULL ) cb(btlecIn, true, &fbb_payload, btlecIn->inFlightRequest.userVar);
}


static void handleBgTimeout(void* userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);

	cxa_logger_warn(&btlecIn->logger, "timeout for  cid:%d  mtd:%d", btlecIn->inFlightRequest.classId, btlecIn->inFlightRequest.methodId);

	// reset paused _before_ we call our callback (so the callback can send a message if it desires)
	cxa_blueGiga_btle_client_cb_onResponse_t cb = btlecIn->inFlightRequest.cb_onResponse;
	cxa_softWatchDog_pause(&btlecIn->inFlightRequest.watchdog);
	if( cb != NULL ) cb(btlecIn, false, NULL, btlecIn->inFlightRequest.userVar);
}


static void handleReadWriteTimeout(void* userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);

	cxa_logger_warn(&btlecIn->logger, "read/write timeout");
	cxa_stateMachine_transition(&btlecIn->stateMachine_currProcedure, PROCSTATE_ERROR);
}


static void protoParseCb_onPacketRx(cxa_fixedByteBuffer_t *const packetIn, void *const userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);

	cxa_blueGiga_msgType_t msgType = getMsgType(packetIn);
	switch( msgType )
	{
		case CXA_BLUEGIGA_MSGTYPE_EVENT:
			handleBgEvent(btlecIn, packetIn);
			break;

		case CXA_BLUEGIGA_MSGTYPE_RESPONSE:
			handleBgResponse(btlecIn, packetIn);
			break;

		default:
			break;
	}
}


static void responseCb_setScanParams(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn)
{
	cxa_assert(btlecIn);

	// we only expect this when we're starting to scan...
	if( cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_conn) != CONNSTATE_SCANNING ) return;

	// check our return value
	uint16_t response;
	if( !wasSuccessfulIn || !cxa_fixedByteBuffer_get_uint16LE(payloadIn, 0, response) || (response != 0) )
	{
		cxa_logger_warn(&btlecIn->logger, "error setting scan params 0x%04X, aborting scan", response);
		cxa_btle_client_notify_scanStart(&btlecIn->super, false);
		cxa_stateMachine_transition(&btlecIn->stateMachine_conn, CONNSTATE_READY);
		return;
	}

	cxa_logger_debug(&btlecIn->logger, "parameters set successfully, starting scan");

	// start scan (discover all devices)
	cxa_fixedByteBuffer_t params;
	uint8_t params_raw[1];
	cxa_fixedByteBuffer_initStd(&params, params_raw);
	cxa_fixedByteBuffer_append_uint8(&params, 2);											// all devices
	cxa_blueGiga_btle_client_sendCommand(btlecIn, CXA_BLUEGIGA_CLASSID_GAP, CXA_BLUEGIGA_METHODID_GAP_DISCOVER, &params, responseCb_discover, NULL);
}


static void responseCb_discover(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn)
{
	cxa_assert(btlecIn);

	// we only expect this when we're starting to scan...
	if( cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_conn) != CONNSTATE_SCANNING ) return;

	// check our return value
	uint16_t response;
	if( !wasSuccessfulIn || !cxa_fixedByteBuffer_get_uint16LE(payloadIn, 0, response) || (response != 0) )
	{
		cxa_logger_warn(&btlecIn->logger, "error starting scan 0x%04X, aborting", response);

		// make sure we stop the scan in case it started and we missed it
		cxa_blueGiga_btle_client_sendCommand(btlecIn, CXA_BLUEGIGA_CLASSID_GAP, CXA_BLUEGIGA_METHODID_GAP_ENDPROCEDURE, NULL, NULL, NULL);

		// notify our clients and transition back to ready
		cxa_btle_client_notify_scanStart(&btlecIn->super, false);
		cxa_stateMachine_transition(&btlecIn->stateMachine_conn, CONNSTATE_READY);
		return;
	}

	cxa_logger_info(&btlecIn->logger, "scan started");
	cxa_btle_client_notify_scanStart(&btlecIn->super, true);
}


static void responseCb_endProcedure(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn)
{
	cxa_assert(btlecIn);

	// we only expect this when we're starting to scan...
	if( cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_conn) != CONNSTATE_SCANNING ) return;

	// check our return value
	uint16_t response;
	if( !wasSuccessfulIn || !cxa_fixedByteBuffer_get_uint16LE(payloadIn, 0, response) || (response != 0) )
	{
		cxa_logger_warn(&btlecIn->logger, "problem stopping scan %d 0x%04X", wasSuccessfulIn, response);
		return;
	}

	cxa_logger_info(&btlecIn->logger, "scan stopped");
	cxa_stateMachine_transition(&btlecIn->stateMachine_conn, CONNSTATE_READY);
}


static void responseCb_connectDirect(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn)
{
	cxa_assert(btlecIn);

	// we only expect this when we're connecting...
	if( cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_conn) != CONNSTATE_CONNECTING ) return;

	// check our return value
	uint16_t response;
	if( !wasSuccessfulIn || !cxa_fixedByteBuffer_get_uint16LE(payloadIn, 0, response) || (response != 0) )
	{
		cxa_logger_warn(&btlecIn->logger, "error starting connection 0x%04X, aborting", response);
		cxa_btle_client_notify_connectionStarted(&btlecIn->super, false);
		cxa_stateMachine_transition(&btlecIn->stateMachine_conn, CONNSTATE_READY);
		return;
	}

	// if we made it here, connection was started successfully, get our connection handle
	if( !cxa_fixedByteBuffer_get_uint8(payloadIn, 2, btlecIn->currConnHandle) )
	{
		cxa_logger_warn(&btlecIn->logger, "error parsing conn response, aborting", response);
		cxa_btle_client_notify_connectionStarted(&btlecIn->super, false);
		cxa_stateMachine_transition(&btlecIn->stateMachine_conn, CONNSTATE_READY);
		return;
	}

	cxa_logger_debug(&btlecIn->logger, "connection started w/ handle %d, waiting for response", btlecIn->currConnHandle);
}


static void responseCb_findByGroupType(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn)
{
	cxa_assert(btlecIn);

	// we only expect this when we're connected...
	if( (cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_conn) != CONNSTATE_CONNECTED) ||
		(cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_currProcedure) != PROCSTATE_RESOLVE_SERVICE_HANDLE) ) return;

	// check our return value
	uint16_t response;
	if( !wasSuccessfulIn || !cxa_fixedByteBuffer_get_uint16LE(payloadIn, 1, response) || (response != 0) )
	{
		cxa_logger_warn(&btlecIn->logger, "error finding by group type 0x%04X, aborting", response);
		cxa_stateMachine_transition(&btlecIn->stateMachine_currProcedure, PROCSTATE_ERROR);
		return;
	}

	cxa_logger_debug(&btlecIn->logger, "find by group type started successfully");
}


static void responseCb_findInformation(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn)
{
	cxa_assert(btlecIn);

	// we only expect this when we're connected...
	if( (cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_conn) != CONNSTATE_CONNECTED) ||
		(cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_currProcedure) != PROCSTATE_RESOLVE_CHARACTERISTIC_HANDLE) ) return;

	// check our return value
	uint16_t response;
	if( !wasSuccessfulIn || !cxa_fixedByteBuffer_get_uint16LE(payloadIn, 1, response) || (response != 0) )
	{
		cxa_logger_warn(&btlecIn->logger, "error finding char. info, aborting", response);
		cxa_stateMachine_transition(&btlecIn->stateMachine_currProcedure, PROCSTATE_ERROR);
		return;
	}

	cxa_logger_debug(&btlecIn->logger, "find info started successfully");
}


static void responseCb_attributeWrite(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn)
{
	cxa_assert(btlecIn);

	// we only expect this when we're connected...
	if( (cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_conn) != CONNSTATE_CONNECTED) ||
		(cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_currProcedure) != PROCSTATE_WRITE_CHARACTERISTIC) ) return;

	// check our return value
	uint16_t response;
	if( !wasSuccessfulIn || !cxa_fixedByteBuffer_get_uint16LE(payloadIn, 1, response) || (response != 0) )
	{
		cxa_logger_warn(&btlecIn->logger, "error writing char., aborting", response);
		cxa_stateMachine_transition(&btlecIn->stateMachine_currProcedure, PROCSTATE_ERROR);
		return;
	}

	cxa_logger_info(&btlecIn->logger, "write completed successfully");
	cxa_stateMachine_transition(&btlecIn->stateMachine_currProcedure, PROCSTATE_IDLE);
}


static void gpioCb_onGpiosConfigured(cxa_blueGiga_btle_client_t* btlecIn, bool wasSuccessfulIn)
{
	cxa_assert(btlecIn);

	if( wasSuccessfulIn ) cxa_logger_debug(&btlecIn->logger, "gpios configured successfully");
	else cxa_logger_warn(&btlecIn->logger, "gpio configuration failed, restarting");
	cxa_stateMachine_transition(&btlecIn->stateMachine_conn, (wasSuccessfulIn ? CONNSTATE_READY : CONNSTATE_RESET));
}


static cxa_btle_client_state_t scm_getState(cxa_btle_client_t *const superIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)superIn;
	cxa_assert(btlecIn);

	cxa_btle_client_state_t retVal = CXA_BTLE_CLIENT_STATE_STARTUP;
	switch( cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_conn) )
	{
		case CONNSTATE_RESET:
		case CONNSTATE_WAIT_BOOT:
		case CONNSTATE_CONFIG_PERIPHS:
			retVal = btlecIn->hasBootFailed ? CXA_BTLE_CLIENT_STATE_STARTUPFAILED : CXA_BTLE_CLIENT_STATE_STARTUP;
			break;

		case CONNSTATE_READY:
			retVal = CXA_BTLE_CLIENT_STATE_READY;
			break;

		case CONNSTATE_SCANNING:
			retVal = CXA_BTLE_CLIENT_STATE_SCANNING;
			break;

		case CONNSTATE_CONNECTING:
			retVal = CXA_BTLE_CLIENT_STATE_CONNECTING;
			break;

		case CONNSTATE_CONNECTED:
			retVal = CXA_BTLE_CLIENT_STATE_CONNECTED;
			break;
	}
	return retVal;
}


static void scm_startScan(cxa_btle_client_t *const superIn, bool isActiveIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)superIn;
	cxa_assert(btlecIn);

	// make sure we're in the proper state
	if( cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_conn) != CONNSTATE_READY )
	{
		cxa_logger_warn(&btlecIn->logger, "not ready to start scan");
		cxa_btle_client_notify_scanStart(&btlecIn->super, false);
		return;
	}

	btlecIn->isActiveScan = isActiveIn;
	cxa_stateMachine_transition(&btlecIn->stateMachine_conn, CONNSTATE_SCANNING);
}


static void scm_stopScan(cxa_btle_client_t *const superIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)superIn;
	cxa_assert(btlecIn);

	if( cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_conn) != CONNSTATE_SCANNING ) return;

	// send our command to stop scanning
	cxa_logger_debug(&btlecIn->logger, "stopping scan");
	cxa_blueGiga_btle_client_sendCommand(btlecIn, CXA_BLUEGIGA_CLASSID_GAP, CXA_BLUEGIGA_METHODID_GAP_ENDPROCEDURE, NULL, responseCb_endProcedure, NULL);
}



static void scm_startConnection(cxa_btle_client_t *const superIn, cxa_eui48_t *const addrIn, bool isRandomAddrIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)superIn;
	cxa_assert(btlecIn);

	// make sure we're in the proper state
	if( cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_conn) != CONNSTATE_READY )
	{
		cxa_logger_warn(&btlecIn->logger, "not ready to connect");
		cxa_btle_client_notify_connectionStarted(&btlecIn->super, false);
		return;
	}

	// save our address info for the connection stage
	memcpy(&btlecIn->connectAddr, addrIn, sizeof(btlecIn->connectAddr));
	btlecIn->isConnectAddrRandom = isRandomAddrIn;

	cxa_stateMachine_transition(&btlecIn->stateMachine_conn, CONNSTATE_CONNECTING);

	return;
}


static void scm_stopConnection(cxa_btle_client_t *const superIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)superIn;
	cxa_assert(btlecIn);

	connState_t currState = cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_conn);
	if( (currState != CONNSTATE_CONNECTING) && (currState != CONNSTATE_CONNECTED) ) return;

	cxa_logger_debug(&btlecIn->logger, "closing connection");

	// send our command to close the connection
	cxa_fixedByteBuffer_t params;
	uint8_t params_raw[1];
	cxa_fixedByteBuffer_initStd(&params, params_raw);
	cxa_fixedByteBuffer_append_uint8(&params, btlecIn->currConnHandle);

	// no response callback since we'll get an event when the disconnect occurs
	cxa_blueGiga_btle_client_sendCommand(btlecIn, CXA_BLUEGIGA_CLASSID_CONNECTION, CXA_BLUEGIGA_METHODID_CONNECTION_DISCONNECT, &params, NULL, NULL);
}


static bool scm_isConnected(cxa_btle_client_t *const superIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)superIn;
	cxa_assert(btlecIn);

	return cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_conn) == CONNSTATE_CONNECTED;
}


static void scm_writeToCharacteristic(cxa_btle_client_t *const superIn,
									  const char *const serviceIdIn,
									  const char *const characteristicIdIn,
									  cxa_fixedByteBuffer_t *const dataIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)superIn;
	cxa_assert(btlecIn);
	cxa_assert(dataIn);

	// make sure we're not in the middle of a procedure
	if( cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_currProcedure) != PROCSTATE_IDLE )
	{
		cxa_logger_warn(&btlecIn->logger, "procedure in progress");
		cxa_stateMachine_transition(&btlecIn->stateMachine_currProcedure, PROCSTATE_ERROR);
		return;
	}

	// make sure we're not sending too much / too little
	size_t numBytesOfData = cxa_fixedByteBuffer_getSize_bytes(dataIn);
	if( (numBytesOfData == 0) || (numBytesOfData > 20) )
	{
		cxa_logger_warn(&btlecIn->logger, "too much/little data to send");
		cxa_stateMachine_transition(&btlecIn->stateMachine_currProcedure, PROCSTATE_ERROR);
		return;
	}

	// save our target service and characteristic
	if( !cxa_btle_uuid_initFromString(&btlecIn->currProcedure.readWriteTargetUuid_service, serviceIdIn) ||
		!cxa_btle_uuid_initFromString(&btlecIn->currProcedure.readWriteTargetUuid_characteristic, characteristicIdIn) )
	{
		cxa_logger_warn(&btlecIn->logger, "invalid service/characteristic uuid");
		cxa_stateMachine_transition(&btlecIn->stateMachine_currProcedure, PROCSTATE_ERROR);
		return;
	}

	cxa_btle_uuid_string_t serviceUuid_str, charUuid_str;
	cxa_btle_uuid_toShortString(&btlecIn->currProcedure.readWriteTargetUuid_service, &serviceUuid_str);
	cxa_btle_uuid_toShortString(&btlecIn->currProcedure.readWriteTargetUuid_characteristic, &charUuid_str);

	cxa_logger_info(&btlecIn->logger, "writing %d bytes to '...%s'::'...%s'", numBytesOfData, serviceUuid_str.str, charUuid_str.str);

	// save our data locally (since it won't be written for a while)
	memcpy(btlecIn->currProcedure.writeData, cxa_fixedByteBuffer_get_pointerToIndex(dataIn, 0), numBytesOfData);
	btlecIn->currProcedure.writeDataLength_bytes = numBytesOfData;

	btlecIn->currProcedure.procedureType = CXA_BLUEGIGA_BTLE_PROCEDURE_TYPE_WRITE;
	cxa_stateMachine_transition(&btlecIn->stateMachine_currProcedure, PROCSTATE_RESOLVE_SERVICE_HANDLE);
}


static void stateCb_conn_reset_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);

	// assert reset line
	cxa_logger_trace(&btlecIn->logger, "asserting reset");
	cxa_gpio_setValue(btlecIn->gpio_reset, 1);

	// reset our protocol parser once
	cxa_protocolParser_reset(&btlecIn->protoParse.super);
}


static void stateCb_conn_reset_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);

	// reset our protocol parser again
	cxa_protocolParser_reset(&btlecIn->protoParse.super);

	// de-assert reset line
	cxa_logger_trace(&btlecIn->logger, "de-asserting reset");
	cxa_gpio_setValue(btlecIn->gpio_reset, 0);
}


static void stateCb_conn_waitBoot_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);

	// failed to receive our boot message
	if( nextStateIdIn == CONNSTATE_RESET )
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


static void stateCb_conn_configPeriphs_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);

	cxa_logger_debug(&btlecIn->logger, "configuring BG gpios");
	cxa_blueGiga_configureGpiosForBlueGiga(btlecIn, gpioCb_onGpiosConfigured);
}


static void stateCb_conn_ready_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);

	// we've just booted, let our listeners know
	if( prevStateIdIn == CONNSTATE_CONFIG_PERIPHS )
	{
		cxa_btle_client_notify_onBecomesReady(&btlecIn->super);
	}
	else if( prevStateIdIn == CONNSTATE_SCANNING )
	{
		cxa_btle_client_notify_scanStop(&btlecIn->super);
	}
	else if( prevStateIdIn == CONNSTATE_CONNECTED )
	{
		cxa_btle_client_notify_connectionClose(&btlecIn->super);
	}
}


static void stateCb_conn_scanning_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);

	cxa_logger_debug(&btlecIn->logger, "setting scan params");

	// set scan parameters
	cxa_fixedByteBuffer_t params;
	uint8_t params_raw[5];
	cxa_fixedByteBuffer_initStd(&params, params_raw);
	cxa_fixedByteBuffer_append_uint16LE(&params, 0x004B);									// scan interval (default)
	cxa_fixedByteBuffer_append_uint16LE(&params, 0x004B);									// scan window (default)
	cxa_fixedByteBuffer_append_uint8(&params, btlecIn->isActiveScan);
	if( !cxa_blueGiga_btle_client_sendCommand(btlecIn, CXA_BLUEGIGA_CLASSID_GAP, CXA_BLUEGIGA_METHODID_GAP_SETSCANPARAMS, &params, responseCb_setScanParams, NULL) )
	{
		cxa_logger_warn(&btlecIn->logger, "failed to start scanning");
		cxa_stateMachine_transition(&btlecIn->stateMachine_conn, CONNSTATE_READY);
	}
}


static void stateCb_conn_scanning_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);
}


static void stateCb_conn_connecting_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);

	cxa_eui48_string_t eui48_str;
	cxa_eui48_toString(&btlecIn->connectAddr, &eui48_str);
	cxa_logger_info(&btlecIn->logger, "connecting to '%s'", eui48_str.str);

	cxa_fixedByteBuffer_t params;
	uint8_t params_raw[15];
	cxa_fixedByteBuffer_initStd(&params, params_raw);
	cxa_fixedByteBuffer_append(&params, btlecIn->connectAddr.bytes, sizeof(btlecIn->connectAddr.bytes));
	cxa_fixedByteBuffer_append_uint8(&params, btlecIn->isConnectAddrRandom);
	cxa_fixedByteBuffer_append_uint16LE(&params, (uint16_t)(CONNECTION_INTERVAL_MIN_MS / 1.25));
	cxa_fixedByteBuffer_append_uint16LE(&params, (uint16_t)(CONNECTION_INTERVAL_MAX_MS / 1.25));
	cxa_fixedByteBuffer_append_uint16LE(&params, (uint16_t)(CONNECTION_TIMEOUT_MS / 10.0));
	cxa_fixedByteBuffer_append_uint16LE(&params, (uint16_t)CONNECTION_LATENCY);
	cxa_blueGiga_btle_client_sendCommand(btlecIn, CXA_BLUEGIGA_CLASSID_GAP, CXA_BLUEGIGA_METHODID_GAP_CONNECT_DIRECT, &params, responseCb_connectDirect, NULL);
}


static void stateCb_conn_connected_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);

	cxa_eui48_string_t eui48_str;
	cxa_eui48_toString(&btlecIn->connectAddr, &eui48_str);
	cxa_logger_info(&btlecIn->logger, "connected to '%s'", eui48_str.str);

	cxa_stateMachine_transitionNow(&btlecIn->stateMachine_currProcedure, PROCSTATE_IDLE);
	cxa_btle_client_notify_connectionStarted(&btlecIn->super, true);
}


static void stateCb_conn_connected_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);

	cxa_eui48_string_t eui48_str;
	cxa_eui48_toString(&btlecIn->connectAddr, &eui48_str);
	cxa_logger_info(&btlecIn->logger, "disconnected from '%s'", eui48_str.str);

	procedureState_t procState = cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_currProcedure);
	if( (procState != PROCSTATE_NOT_READY) &&
		(procState != PROCSTATE_IDLE) &&
		(procState != PROCSTATE_ERROR) )
	{
		cxa_logger_debug(&btlecIn->logger, "ending procedure in-progress");
		cxa_stateMachine_transition(&btlecIn->stateMachine_currProcedure, PROCSTATE_ERROR);
		return;
	}
	else
	{
		cxa_stateMachine_transitionNow(&btlecIn->stateMachine_currProcedure, PROCSTATE_NOT_READY);
	}
}


static void stateCb_currProc_idle_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);

	// do any success notifications first
	switch( (procedureState_t)prevStateIdIn )
	{
		case PROCSTATE_READ_CHARACTERISTIC:
			cxa_btle_client_notify_readComplete(&btlecIn->super,
					 	 	 	 	 	 	    &btlecIn->currProcedure.readWriteTargetUuid_service,
												&btlecIn->currProcedure.readWriteTargetUuid_characteristic,
												true);
			break;

		case PROCSTATE_WRITE_CHARACTERISTIC:
			cxa_btle_client_notify_writeComplete(&btlecIn->super,
												 &btlecIn->currProcedure.readWriteTargetUuid_service,
												 &btlecIn->currProcedure.readWriteTargetUuid_characteristic,
												 true);
			break;

		default:
			break;
	}

	cxa_logger_debug(&btlecIn->logger, "procedure is idle");

	// make sure our watchdog is paused
	cxa_softWatchDog_pause(&btlecIn->currProcedure.watchdog);
}


static void stateCb_currProc_resolveService_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);

	cxa_logger_debug(&btlecIn->logger, "resolving service handle");

	// set our internal state appropriately
	btlecIn->currProcedure.serviceHandle_start = 0;
	btlecIn->currProcedure.serviceHandle_end = 0;

	// need to send attclient_read_by_group_type to start figuring out our handle
	cxa_fixedByteBuffer_t params;
	uint8_t params_raw[8];
	cxa_fixedByteBuffer_initStd(&params, params_raw);
	cxa_fixedByteBuffer_append_uint8(&params, btlecIn->currConnHandle);
	cxa_fixedByteBuffer_append_uint16LE(&params, 0x0001);
	cxa_fixedByteBuffer_append_uint16LE(&params, 0xFFFF);

	cxa_fixedByteBuffer_append_uint8(&params, 2);
	cxa_fixedByteBuffer_append_uint16LE(&params, 0x2800);
	cxa_blueGiga_btle_client_sendCommand(btlecIn, CXA_BLUEGIGA_CLASSID_ATTR_CLIENT, CXA_BLUEGIGA_METHODID_ATTR_CLIENT_READ_BY_GROUP_TYPE, &params, responseCb_findByGroupType, NULL);

	// reset our watchdog so we can eventually timeout if needed
	cxa_softWatchDog_kick(&btlecIn->currProcedure.watchdog);
}


static void stateCb_currProc_resolveCharacteristic_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);

	cxa_logger_debug(&btlecIn->logger, "resolving characteristic handle");

	// set our internal state appropriately
	btlecIn->currProcedure.characteristicHandle = 0;

	// need to send attclient_find_information to start figuring out our handle
	cxa_fixedByteBuffer_t params;
	uint8_t params_raw[5];
	cxa_fixedByteBuffer_initStd(&params, params_raw);
	cxa_fixedByteBuffer_append_uint8(&params, btlecIn->currConnHandle);
	cxa_fixedByteBuffer_append_uint16LE(&params, btlecIn->currProcedure.serviceHandle_start);
	cxa_fixedByteBuffer_append_uint16LE(&params, btlecIn->currProcedure.serviceHandle_end);
	cxa_blueGiga_btle_client_sendCommand(btlecIn, CXA_BLUEGIGA_CLASSID_ATTR_CLIENT, CXA_BLUEGIGA_METHODID_ATTR_CLIENT_FIND_INFO, &params, responseCb_findInformation, NULL);

	// reset our watchdog to give us more time
	cxa_softWatchDog_kick(&btlecIn->currProcedure.watchdog);
}


static void stateCb_currProc_readCharacteristic_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);

	cxa_logger_debug(&btlecIn->logger, "read char");
}


static void stateCb_currProc_writeCharacteristic_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);

	cxa_logger_debug(&btlecIn->logger, "writing %d bytes", btlecIn->currProcedure.writeDataLength_bytes);

	cxa_fixedByteBuffer_t params;
	uint8_t params_raw[4+btlecIn->currProcedure.writeDataLength_bytes];
	cxa_fixedByteBuffer_initStd(&params, params_raw);
	cxa_fixedByteBuffer_append_uint8(&params, btlecIn->currConnHandle);
	cxa_fixedByteBuffer_append_uint16LE(&params, btlecIn->currProcedure.characteristicHandle);

	cxa_fixedByteBuffer_append_uint8(&params, btlecIn->currProcedure.writeDataLength_bytes);
	cxa_fixedByteBuffer_append(&params, btlecIn->currProcedure.writeData, btlecIn->currProcedure.writeDataLength_bytes);
	cxa_blueGiga_btle_client_sendCommand(btlecIn, CXA_BLUEGIGA_CLASSID_ATTR_CLIENT, CXA_BLUEGIGA_METHODID_ATTR_CLIENT_ATTRIBUTE_WRITE, &params, responseCb_attributeWrite, NULL);
}


static void stateCb_currProc_error_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);

	cxa_logger_debug(&btlecIn->logger, "procedure encountered error");
	switch( btlecIn->currProcedure.procedureType )
	{
		case CXA_BLUEGIGA_BTLE_PROCEDURE_TYPE_READ:
			cxa_btle_client_notify_readComplete(&btlecIn->super,
					 	 	 	 	 	 	 	&btlecIn->currProcedure.readWriteTargetUuid_service,
												&btlecIn->currProcedure.readWriteTargetUuid_characteristic,
												false);
			break;

		case CXA_BLUEGIGA_BTLE_PROCEDURE_TYPE_WRITE:
			cxa_btle_client_notify_writeComplete(&btlecIn->super,
					 	 	 	 	 	 	 	 &btlecIn->currProcedure.readWriteTargetUuid_service,
												 &btlecIn->currProcedure.readWriteTargetUuid_characteristic,
												 false);
			break;
	}

	// if we're still connected, we can continue...if the error was caused by a disconnection
	// then we'll now be in the "not-ready" state
	if( cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_conn) == CONNSTATE_CONNECTED )
	{
		cxa_stateMachine_transition(&btlecIn->stateMachine_currProcedure, PROCSTATE_IDLE);
	}
	else
	{
		cxa_stateMachine_transition(&btlecIn->stateMachine_currProcedure, PROCSTATE_NOT_READY);
	}
}


