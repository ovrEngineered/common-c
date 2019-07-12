/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
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

#define READWRITE_TIMEOUT_MS			2000

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
}connState_t;


// ******** local function prototypes ********
static cxa_blueGiga_msgType_t getMsgType(cxa_fixedByteBuffer_t *const fbbIn);
static cxa_blueGiga_classId_t getClassId(cxa_fixedByteBuffer_t *const fbbIn);
static cxa_blueGiga_methodId_t getMethod(cxa_fixedByteBuffer_t *const fbbIn);

static void handleBgEvent(cxa_blueGiga_btle_client_t *const btlecIn, cxa_fixedByteBuffer_t *const packetIn);
static void handleBgResponse(cxa_blueGiga_btle_client_t *const btlecIn, cxa_fixedByteBuffer_t *const packetIn);
static void handleBgTimeout(void* userVarIn);

static void protoParseCb_onPacketRx(cxa_fixedByteBuffer_t *const packetIn, void *const userVarIn);

static void responseCb_setScanParams(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn);
static void responseCb_discover(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn);
static void responseCb_endProcedure(cxa_blueGiga_btle_client_t *const btlecIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const payloadIn, void* userVarIn);

static void gpioCb_onGpiosConfigured(cxa_blueGiga_btle_client_t* btlecIn, bool wasSuccessfulIn);

static cxa_btle_central_state_t scm_getState(cxa_btle_central_t *const superIn);
static void scm_startScan(cxa_btle_central_t *const superIn, bool isActiveIn);
static void scm_stopScan(cxa_btle_central_t *const superIn);

static void stateCb_conn_reset_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_conn_reset_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_conn_waitBoot_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_conn_configPeriphs_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_conn_ready_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_conn_scanning_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_conn_scanning_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);


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

	// initialize our superclass
	cxa_btle_central_init(&btlecIn->super, scm_getState,
						 scm_startScan, scm_stopScan,
						 NULL, NULL,
						 NULL, NULL, NULL);

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
	cxa_stateMachine_setInitialState(&btlecIn->stateMachine_conn, CONNSTATE_RESET);
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
		cxa_btle_central_notify_advertRx(&btlecIn->super, &rxPacket);
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
		cxa_btle_central_notify_scanStart(&btlecIn->super, false);
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
		cxa_btle_central_notify_scanStart(&btlecIn->super, false);
		cxa_stateMachine_transition(&btlecIn->stateMachine_conn, CONNSTATE_READY);
		return;
	}

	cxa_logger_info(&btlecIn->logger, "scan started");
	cxa_btle_central_notify_scanStart(&btlecIn->super, true);
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


static void gpioCb_onGpiosConfigured(cxa_blueGiga_btle_client_t* btlecIn, bool wasSuccessfulIn)
{
	cxa_assert(btlecIn);

	if( wasSuccessfulIn ) cxa_logger_debug(&btlecIn->logger, "gpios configured successfully");
	else cxa_logger_warn(&btlecIn->logger, "gpio configuration failed, restarting");
	cxa_stateMachine_transition(&btlecIn->stateMachine_conn, (wasSuccessfulIn ? CONNSTATE_READY : CONNSTATE_RESET));
}


static cxa_btle_central_state_t scm_getState(cxa_btle_central_t *const superIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)superIn;
	cxa_assert(btlecIn);

	cxa_btle_central_state_t retVal = CXA_BTLE_CENTRAL_STATE_STARTUP;
	switch( cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_conn) )
	{
		case CONNSTATE_RESET:
		case CONNSTATE_WAIT_BOOT:
		case CONNSTATE_CONFIG_PERIPHS:
			retVal = btlecIn->hasBootFailed ? CXA_BTLE_CENTRAL_STATE_STARTUPFAILED : CXA_BTLE_CENTRAL_STATE_STARTUP;
			break;

		case CONNSTATE_READY:
		case CONNSTATE_SCANNING:
			retVal = CXA_BTLE_CENTRAL_STATE_READY;
			break;
	}
	return retVal;
}


static void scm_startScan(cxa_btle_central_t *const superIn, bool isActiveIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)superIn;
	cxa_assert(btlecIn);

	// make sure we're in the proper state
	if( cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_conn) != CONNSTATE_READY )
	{
		cxa_logger_warn(&btlecIn->logger, "not ready to start scan");
		cxa_btle_central_notify_scanStart(&btlecIn->super, false);
		return;
	}

	btlecIn->isActiveScan = isActiveIn;
	cxa_stateMachine_transition(&btlecIn->stateMachine_conn, CONNSTATE_SCANNING);
}


static void scm_stopScan(cxa_btle_central_t *const superIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)superIn;
	cxa_assert(btlecIn);

	if( cxa_stateMachine_getCurrentState(&btlecIn->stateMachine_conn) != CONNSTATE_SCANNING ) return;

	// send our command to stop scanning
	cxa_logger_debug(&btlecIn->logger, "stopping scan");
	cxa_blueGiga_btle_client_sendCommand(btlecIn, CXA_BLUEGIGA_CLASSID_GAP, CXA_BLUEGIGA_METHODID_GAP_ENDPROCEDURE, NULL, responseCb_endProcedure, NULL);
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
		cxa_btle_central_notify_onFailedInit(&btlecIn->super, true);
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
		cxa_btle_central_notify_onBecomesReady(&btlecIn->super);
	}
	else if( prevStateIdIn == CONNSTATE_SCANNING )
	{
		cxa_btle_central_notify_scanStop(&btlecIn->super);
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
