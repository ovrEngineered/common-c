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
#include <cxa_blueGiga_types.h>
#include <cxa_console.h>
#include <cxa_delay.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define RESET_TIME_MS			1000
#define WAIT_BOOT_TIME_MS		5000


// ******** local type definitions ********
typedef enum
{
	STATE_RESET,
	STATE_WAIT_BOOT,
	STATE_IDLE,
	STATE_SCANNING_SETUP_PARAMS,
	STATE_SCANNING
}state_t;


// ******** local function prototypes ********
static cxa_blueGiga_msgType_t getMsgType(cxa_fixedByteBuffer_t *const fbbIn);
static cxa_blueGiga_classId_t getClassId(cxa_fixedByteBuffer_t *const fbbIn);
static uint8_t getCommandId(cxa_fixedByteBuffer_t *const fbbIn);

static void protoParseCb_onPacketRx(cxa_fixedByteBuffer_t *const packetIn, void *const userVarIn);

static void scm_startScan(cxa_btle_client_t *const superIn, bool isActiveIn);
static void scm_stopScan(cxa_btle_client_t *const superIn);
static bool scm_isScanning(cxa_btle_client_t *const superIn);

static void stateCb_reset_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_reset_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_scanningSetup_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_scanning_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_scanning_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_blueGiga_btle_client_init(cxa_blueGiga_btle_client_t *const btlecIn, cxa_ioStream_t *const iosIn, cxa_gpio_t *const gpio_resetIn)
{
	cxa_assert(btlecIn);
	cxa_assert(iosIn);
	cxa_assert(gpio_resetIn);

	// save our references
	btlecIn->gpio_reset = gpio_resetIn;

	// initialize our superclass
	cxa_btle_client_init(&btlecIn->super, scm_startScan, scm_stopScan, scm_isScanning);

	// and our logger
	cxa_logger_init(&btlecIn->logger, "btleClient");

	// and our buffers
	cxa_fixedByteBuffer_initStd(&btlecIn->fbb_tx, btlecIn->fbb_tx_raw);
	cxa_fixedByteBuffer_initStd(&btlecIn->fbb_rx, btlecIn->fbb_rx_raw);

	// our protocol parser
	cxa_protocolParser_bgapi_init(&btlecIn->protoParse, iosIn, &btlecIn->fbb_rx);
	cxa_protocolParser_addPacketListener(&btlecIn->protoParse.super, protoParseCb_onPacketRx, (void*)btlecIn);

	// and our state machine
	cxa_stateMachine_init(&btlecIn->stateMachine, "blueGiga");
	cxa_stateMachine_addState_timed(&btlecIn->stateMachine, STATE_RESET, "reset", STATE_WAIT_BOOT, RESET_TIME_MS, stateCb_reset_enter, NULL, stateCb_reset_leave, (void*)btlecIn);
	cxa_stateMachine_addState_timed(&btlecIn->stateMachine, STATE_WAIT_BOOT, "waitBoot", STATE_RESET, WAIT_BOOT_TIME_MS, NULL, NULL, NULL, (void*)btlecIn);
	cxa_stateMachine_addState(&btlecIn->stateMachine, STATE_IDLE, "idle", NULL, NULL, NULL, (void*)btlecIn);
	cxa_stateMachine_addState(&btlecIn->stateMachine, STATE_SCANNING_SETUP_PARAMS, "scanningSetup", stateCb_scanningSetup_enter, NULL, NULL, (void*)btlecIn);
	cxa_stateMachine_addState(&btlecIn->stateMachine, STATE_SCANNING, "scanning", stateCb_scanning_enter, NULL, stateCb_scanning_leave, (void*)btlecIn);
	cxa_stateMachine_setInitialState(&btlecIn->stateMachine, STATE_RESET);
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


static uint8_t getCommandId(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);

	uint8_t commandId_raw;
	if( !cxa_fixedByteBuffer_get_uint8(fbbIn, 2, commandId_raw) ) return 0xFF;

	return commandId_raw;
}


static void protoParseCb_onPacketRx(cxa_fixedByteBuffer_t *const packetIn, void *const userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);

	cxa_blueGiga_msgType_t msgType = getMsgType(packetIn);
	cxa_blueGiga_classId_t classId = getClassId(packetIn);
	uint8_t commandId = getCommandId(packetIn);

//	cxa_logger_debug(&btlecIn->logger, "got packet  %d %d %d  %d bytes", msgType, classId, commandId, cxa_fixedByteBuffer_getSize_bytes(packetIn));

	if( (msgType == CXA_BLUEGIGA_MSGTYPE_EVENT) && (classId == CXA_BLUEGIGA_CLASSID_SYSTEM) && (commandId == 0x00) )
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

		cxa_logger_debug(&btlecIn->logger, "boot  sw: %d.%d.%d  prot: %d  hw: %d",
				sw_major, sw_minor,patch, protocol, hw);

		cxa_stateMachine_transition(&btlecIn->stateMachine, STATE_IDLE);
	}
	else if( (msgType == CXA_BLUEGIGA_MSGTYPE_EVENT) && (classId == CXA_BLUEGIGA_CLASSID_GAP) && (commandId == 0x06) )
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
		if( !cxa_fixedByteBuffer_get(packetIn, 6, false, rxPacket.addr, sizeof(rxPacket.addr)) ) return;
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

		cxa_logger_debug(&btlecIn->logger, "adv from %02X:%02X:%02X:%02X:%02X:%02X(%s)  %ddBm  %d fields",
						rxPacket.addr[5], rxPacket.addr[4], rxPacket.addr[3],
						rxPacket.addr[2], rxPacket.addr[1], rxPacket.addr[0],
						rxPacket.isRandomAddress ? "r" : "p",
						rxPacket.rssi, cxa_array_getSize_elems(&rxPacket.advFields));

		// notify our listeners
		cxa_btle_client_notify_advertRx(&btlecIn->super, &rxPacket);
	}
}


static void scm_startScan(cxa_btle_client_t *const superIn, bool isActiveIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)superIn;
	cxa_assert(btlecIn);

	// make sure we're in the proper state
	if( cxa_stateMachine_getCurrentState(&btlecIn->stateMachine) != STATE_IDLE )
	{
		cxa_logger_warn(&btlecIn->logger, "not ready to start scan");
		cxa_btle_client_notify_scanStartFail(&btlecIn->super);
		return;
	}

	btlecIn->isActiveScan = isActiveIn;
	cxa_stateMachine_transition(&btlecIn->stateMachine, STATE_SCANNING_SETUP_PARAMS);
}


static void scm_stopScan(cxa_btle_client_t *const superIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)superIn;
	cxa_assert(btlecIn);

	state_t currState = cxa_stateMachine_getCurrentState(&btlecIn->stateMachine);
	if( (currState == STATE_SCANNING_SETUP_PARAMS) ||
		(currState == STATE_SCANNING) )
	{
		cxa_stateMachine_transition(&btlecIn->stateMachine, STATE_IDLE);
	}
}


static bool scm_isScanning(cxa_btle_client_t *const superIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)superIn;
	cxa_assert(btlecIn);

	return (cxa_stateMachine_getCurrentState(&btlecIn->stateMachine) == STATE_SCANNING);
}


static void stateCb_reset_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);

	cxa_gpio_setValue(btlecIn->gpio_reset, 1);
}


static void stateCb_reset_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);

	cxa_gpio_setValue(btlecIn->gpio_reset, 0);
}


static void stateCb_scanningSetup_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);

	// set scan parameters
	cxa_fixedByteBuffer_clear(&btlecIn->fbb_tx);
	cxa_fixedByteBuffer_append_uint8(&btlecIn->fbb_tx, 0);
	cxa_fixedByteBuffer_append_uint8(&btlecIn->fbb_tx, 5);
	cxa_fixedByteBuffer_append_uint8(&btlecIn->fbb_tx, 6);
	cxa_fixedByteBuffer_append_uint8(&btlecIn->fbb_tx, 7);
	cxa_fixedByteBuffer_append_uint16LE(&btlecIn->fbb_tx, 0x004B);
	cxa_fixedByteBuffer_append_uint16LE(&btlecIn->fbb_tx, 0x0032);
	cxa_fixedByteBuffer_append_uint8(&btlecIn->fbb_tx, btlecIn->isActiveScan);
	cxa_protocolParser_writePacket(&btlecIn->protoParse.super, &btlecIn->fbb_tx);

	// @TODO wait for response
	cxa_stateMachine_transition(&btlecIn->stateMachine, STATE_SCANNING);
}


static void stateCb_scanning_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);

	cxa_logger_debug(&btlecIn->logger, "starting scan");

	// start scan (discover all devices)
	cxa_fixedByteBuffer_clear(&btlecIn->fbb_tx);
	cxa_fixedByteBuffer_append_uint8(&btlecIn->fbb_tx, 0);
	cxa_fixedByteBuffer_append_uint8(&btlecIn->fbb_tx, 1);
	cxa_fixedByteBuffer_append_uint8(&btlecIn->fbb_tx, 6);
	cxa_fixedByteBuffer_append_uint8(&btlecIn->fbb_tx, 2);
	cxa_fixedByteBuffer_append_uint8(&btlecIn->fbb_tx, 2);
	cxa_protocolParser_writePacket(&btlecIn->protoParse.super, &btlecIn->fbb_tx);

	cxa_delay_ms(100);
}


static void stateCb_scanning_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	cxa_blueGiga_btle_client_t* btlecIn = (cxa_blueGiga_btle_client_t*)userVarIn;
	cxa_assert(btlecIn);

	cxa_logger_debug(&btlecIn->logger, "stopping scan");

	// end procedure
	cxa_fixedByteBuffer_clear(&btlecIn->fbb_tx);
	cxa_fixedByteBuffer_append_uint8(&btlecIn->fbb_tx, 0);
	cxa_fixedByteBuffer_append_uint8(&btlecIn->fbb_tx, 0);
	cxa_fixedByteBuffer_append_uint8(&btlecIn->fbb_tx, 6);
	cxa_fixedByteBuffer_append_uint8(&btlecIn->fbb_tx, 4);
	cxa_protocolParser_writePacket(&btlecIn->protoParse.super, &btlecIn->fbb_tx);
}
