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
#include "cxa_protocolParser_bgapi.h"


// ******** includes ********
#include <cxa_assert.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define MAX_NUM_RX_BYTES_PER_UPDATE		16
#define RECEPTION_TIMEOUT_MS			5000


// ******** local type definitions ********
typedef enum
{
	RX_STATE_IDLE,
	RX_STATE_WAIT_PACKET_START,
	RX_STATE_WAIT_PACKETRX,
	RX_STATE_PROCESS_PACKET,
	RX_STATE_ERROR
}rxState_t;


// ******** local function prototypes ********
static uint16_t getExpectedPayloadLength_bytes(cxa_fixedByteBuffer_t *const fbbIn);

static bool scm_isInErrorState(cxa_protocolParser_t *const superIn);
static bool scm_canSetBuffer(cxa_protocolParser_t *const superIn);
static void scm_gotoIdle(cxa_protocolParser_t *const superIn);
static bool scm_writeBytes(cxa_protocolParser_t *const superIn, cxa_fixedByteBuffer_t *const fbbIn);

static void stateCb_idle_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_idle_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_idle_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_waitPacketStart_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_waitPacketStart_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_waitPacketRx_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_waitPacketRx_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_processPacket_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_error_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_protocolParser_bgapi_init(cxa_protocolParser_bgapi_t *const ppIn, cxa_ioStream_t *const ioStreamIn, cxa_fixedByteBuffer_t *const buffIn)
{
	cxa_assert(ppIn);
	cxa_assert(ioStreamIn);

	// initialize our super class
	cxa_protocolParser_init(&ppIn->super, ioStreamIn, buffIn, scm_isInErrorState, scm_canSetBuffer, scm_gotoIdle, scm_writeBytes);

	// setup our state machine
	cxa_stateMachine_init(&ppIn->stateMachine, "bgapiPP");
	cxa_stateMachine_addState(&ppIn->stateMachine, RX_STATE_IDLE, "idle", stateCb_idle_enter, stateCb_idle_state, stateCb_idle_leave, (void*)ppIn);
	cxa_stateMachine_addState(&ppIn->stateMachine, RX_STATE_WAIT_PACKET_START, "waitStart", stateCb_waitPacketStart_enter, stateCb_waitPacketStart_state, NULL, (void*)ppIn);
	cxa_stateMachine_addState(&ppIn->stateMachine, RX_STATE_WAIT_PACKETRX, "waitRx", stateCb_waitPacketRx_enter, stateCb_waitPacketRx_state, NULL, (void*)ppIn);
	cxa_stateMachine_addState(&ppIn->stateMachine, RX_STATE_PROCESS_PACKET, "processPacket", stateCb_processPacket_enter, NULL, NULL, (void*)ppIn);
	cxa_stateMachine_addState(&ppIn->stateMachine, RX_STATE_ERROR, "error", stateCb_error_enter, NULL, NULL, (void*)ppIn);
	cxa_stateMachine_setInitialState(&ppIn->stateMachine, RX_STATE_IDLE);
}


// ******** local function implementations ********
static uint16_t getExpectedPayloadLength_bytes(cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(fbbIn);

	if( cxa_fixedByteBuffer_getSize_bytes(fbbIn) < 2 ) return UINT16_MAX;

	uint8_t lenHigh, lenLow;
	cxa_fixedByteBuffer_get_uint8(fbbIn, 0, lenHigh);
	cxa_fixedByteBuffer_get_uint8(fbbIn, 1, lenLow);

	return (((uint16_t)(lenHigh & 0x07)) << 8) | ((uint16_t)lenLow);

}


static bool scm_isInErrorState(cxa_protocolParser_t *const superIn)
{
	cxa_protocolParser_bgapi_t* ppIn = (cxa_protocolParser_bgapi_t*)superIn;
	cxa_assert(ppIn);

	return (cxa_stateMachine_getCurrentState(&ppIn->stateMachine) == RX_STATE_ERROR);
}


static bool scm_canSetBuffer(cxa_protocolParser_t *const superIn)
{
	cxa_protocolParser_bgapi_t* ppIn = (cxa_protocolParser_bgapi_t*)superIn;
	cxa_assert(ppIn);

	rxState_t currState = (rxState_t)cxa_stateMachine_getCurrentState(&ppIn->stateMachine);
	return (currState == RX_STATE_PROCESS_PACKET) || (currState == RX_STATE_IDLE);
}


static void scm_gotoIdle(cxa_protocolParser_t *const superIn)
{
	cxa_protocolParser_bgapi_t* ppIn = (cxa_protocolParser_bgapi_t*)superIn;
	cxa_assert(ppIn);

	cxa_stateMachine_transitionNow(&ppIn->stateMachine, RX_STATE_IDLE);
	cxa_assert( cxa_stateMachine_getCurrentState(&ppIn->stateMachine) == RX_STATE_IDLE );
}


static bool scm_writeBytes(cxa_protocolParser_t *const superIn, cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_protocolParser_bgapi_t* ppIn = (cxa_protocolParser_bgapi_t*)superIn;
	cxa_assert(ppIn);

	if( !cxa_ioStream_writeByte(ppIn->super.ioStream, cxa_fixedByteBuffer_getSize_bytes(fbbIn)) ) return false;
	return cxa_ioStream_writeFixedByteBuffer(ppIn->super.ioStream, fbbIn);
}


static void stateCb_idle_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_protocolParser_bgapi_t* ppIn = (cxa_protocolParser_bgapi_t*)userVarIn;
	cxa_assert(ppIn);

	cxa_logger_info(&ppIn->super.logger, "becoming idle");
}


static void stateCb_idle_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_bgapi_t* ppIn = (cxa_protocolParser_bgapi_t*)userVarIn;
	cxa_assert(ppIn);

	// if we have a bound ioStream and a buffer, become active
	if( cxa_ioStream_isBound(ppIn->super.ioStream) && (ppIn->super.currBuffer != NULL) )
	{
		cxa_stateMachine_transition(&ppIn->stateMachine, RX_STATE_WAIT_PACKET_START);
		return;
	}
}


static void stateCb_idle_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	cxa_protocolParser_bgapi_t* ppIn = (cxa_protocolParser_bgapi_t*)userVarIn;
	cxa_assert(ppIn);

	cxa_logger_info(&ppIn->super.logger, "becoming active");
}


static void stateCb_waitPacketStart_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_protocolParser_bgapi_t* ppIn = (cxa_protocolParser_bgapi_t*)userVarIn;
	cxa_assert(ppIn);

	// clear our rx buffer
	cxa_fixedByteBuffer_clear(ppIn->super.currBuffer);
}


static void stateCb_waitPacketStart_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_bgapi_t* ppIn = (cxa_protocolParser_bgapi_t*)userVarIn;
	cxa_assert(ppIn);

	// try to receive a byte
	uint8_t rxByte;
	for( uint8_t i = 0; i < MAX_NUM_RX_BYTES_PER_UPDATE; i++ )
	{
		cxa_ioStream_readStatus_t readStat = cxa_ioStream_readByte(ppIn->super.ioStream, &rxByte);
		if( readStat == CXA_IOSTREAM_READSTAT_ERROR )
		{
			cxa_stateMachine_transition(&ppIn->stateMachine, RX_STATE_ERROR);
			return;
		}
		else if( readStat == CXA_IOSTREAM_READSTAT_GOTDATA )
		{
			// make sure the first byte makes sense
			if( ((rxByte >> 3) & 0xF) == 0 )
			{
				// got a valid byte
				cxa_fixedByteBuffer_append_uint8(ppIn->super.currBuffer, rxByte);
				cxa_stateMachine_transition(&ppIn->stateMachine, RX_STATE_WAIT_PACKETRX);
				return;
			}
		}
		else
		{
			// no data to receive...leave our loop (to keep runLoop running nicely)
			break;
		}
	}
}


static void stateCb_waitPacketRx_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_protocolParser_bgapi_t* ppIn = (cxa_protocolParser_bgapi_t*)userVarIn;
	cxa_assert(ppIn);

	// need to reset our reception timeout
	cxa_timeDiff_setStartTime_now(&ppIn->super.td_timeout);
}


static void stateCb_waitPacketRx_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_bgapi_t* ppIn = (cxa_protocolParser_bgapi_t*)userVarIn;
	cxa_assert(ppIn);

	// check our reception timeout
	if( cxa_timeDiff_isElapsed_ms(&ppIn->super.td_timeout, RECEPTION_TIMEOUT_MS) )
	{
		cxa_protocolParser_notify_receptionTimeout(&ppIn->super);
		cxa_stateMachine_transition(&ppIn->stateMachine, RX_STATE_WAIT_PACKET_START);
		return;
	}

	// try to receive a byte
	uint8_t rxByte;
	for( uint8_t i = 0; i < MAX_NUM_RX_BYTES_PER_UPDATE; i++ )
	{
		cxa_ioStream_readStatus_t readStat = cxa_ioStream_readByte(ppIn->super.ioStream, &rxByte);
		if( readStat == CXA_IOSTREAM_READSTAT_ERROR )
		{
			cxa_stateMachine_transition(&ppIn->stateMachine, RX_STATE_ERROR);
			return;
		}
		else if( readStat == CXA_IOSTREAM_READSTAT_GOTDATA )
		{
			// add our byte and reset our reception timeout
			cxa_fixedByteBuffer_append_uint8(ppIn->super.currBuffer, rxByte);
			cxa_timeDiff_setStartTime_now(&ppIn->super.td_timeout);

			// now see if we have a complete packet
			size_t fbbSize_bytes = cxa_fixedByteBuffer_getSize_bytes(ppIn->super.currBuffer);
			size_t expectedPacketSize_bytes = getExpectedPayloadLength_bytes(ppIn->super.currBuffer) + 4;

			if( fbbSize_bytes >= expectedPacketSize_bytes )
			{
				// we have enough bytes...process the message
				cxa_stateMachine_transition(&ppIn->stateMachine, RX_STATE_PROCESS_PACKET);
				return;
			}
		}
		else
		{
			// no data to receive...leave our loop (to keep runLoop running nicely)
			break;
		}
	}
}


static void stateCb_processPacket_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_protocolParser_bgapi_t* ppIn = (cxa_protocolParser_bgapi_t*)userVarIn;
	cxa_assert(ppIn);

	//cxa_logger_stepDebug_memDump("got message ", cxa_fixedByteBuffer_get_pointerToIndex(&fbb, 0), cxa_fixedByteBuffer_getSize_bytes(&fbb));
	// valid message...notify our listeners
	cxa_protocolParser_notify_packetReceived(&ppIn->super, ppIn->super.currBuffer);

	// no matter what, we'll reset and wait for more data
	cxa_stateMachine_transition(&ppIn->stateMachine, RX_STATE_WAIT_PACKET_START);
}


static void stateCb_error_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_protocolParser_bgapi_t* ppIn = (cxa_protocolParser_bgapi_t*)userVarIn;
	cxa_assert(ppIn);

	cxa_protocolParser_notify_ioException(&ppIn->super);
}
