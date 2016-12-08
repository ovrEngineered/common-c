/**
 * @copyright 2015 opencxa.org
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
#include "cxa_protocolParser_cleProto.h"


// ******** includes ********
#include <cxa_assert.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_INFO
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define MAX_NUM_RX_BYTES_PER_UPDATE		16
#define RECEPTION_TIMEOUT_MS			5000


// ******** local type definitions ********
typedef enum
{
	RX_STATE_IDLE,
	RX_STATE_WAIT_0x80,
	RX_STATE_WAIT_0x81,
	RX_STATE_WAIT_LEN,
	RX_STATE_WAIT_DATA_BYTES,
	RX_STATE_PROCESS_PACKET,
	RX_STATE_ERROR
}rxState_t;


// ******** local function prototypes ********
static bool scm_isInErrorState(cxa_protocolParser_t *const superIn);
static bool scm_canSetBuffer(cxa_protocolParser_t *const superIn);
static void scm_gotoIdle(cxa_protocolParser_t *const superIn);
static bool scm_writeBytes(cxa_protocolParser_t *const superIn, cxa_fixedByteBuffer_t *const fbbIn);


static void rxState_cb_idle_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void rxState_cb_idle_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_idle_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void rxState_cb_wait0x80_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_wait0x81_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_waitLen_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_waitDataBytes_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_processPacket_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_error_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_protocolParser_cleProto_init(cxa_protocolParser_cleProto_t *const clePpIn, cxa_ioStream_t *const ioStreamIn, cxa_fixedByteBuffer_t *const buffIn)
{
	cxa_assert(clePpIn);
	cxa_assert(ioStreamIn);

	// initialize our super class
	cxa_protocolParser_init(&clePpIn->super, ioStreamIn, buffIn, scm_isInErrorState, scm_canSetBuffer, scm_gotoIdle, scm_writeBytes);

	// setup our state machine
	cxa_stateMachine_init(&clePpIn->stateMachine, "protocolParser");
	cxa_stateMachine_addState(&clePpIn->stateMachine, RX_STATE_IDLE, "idle", rxState_cb_idle_enter, rxState_cb_idle_state, rxState_cb_idle_leave, (void*)clePpIn);
	cxa_stateMachine_addState(&clePpIn->stateMachine, RX_STATE_WAIT_0x80, "wait_0x80", NULL, rxState_cb_wait0x80_state, NULL, (void*)clePpIn);
	cxa_stateMachine_addState(&clePpIn->stateMachine, RX_STATE_WAIT_0x81, "wait_0x81", NULL, rxState_cb_wait0x81_state, NULL, (void*)clePpIn);
	cxa_stateMachine_addState(&clePpIn->stateMachine, RX_STATE_WAIT_LEN, "wait_len", NULL, rxState_cb_waitLen_state, NULL, (void*)clePpIn);
	cxa_stateMachine_addState(&clePpIn->stateMachine, RX_STATE_WAIT_DATA_BYTES, "wait_dataBytes", NULL, rxState_cb_waitDataBytes_state, NULL, (void*)clePpIn);
	cxa_stateMachine_addState(&clePpIn->stateMachine, RX_STATE_PROCESS_PACKET, "processPacket", NULL, rxState_cb_processPacket_state, NULL, (void*)clePpIn);
	cxa_stateMachine_addState(&clePpIn->stateMachine, RX_STATE_ERROR, "error", rxState_cb_error_enter, NULL, NULL, (void*)clePpIn);
	cxa_stateMachine_setInitialState(&clePpIn->stateMachine, RX_STATE_IDLE);
}


// ******** local function implementations ********
static bool scm_isInErrorState(cxa_protocolParser_t *const superIn)
{
	cxa_protocolParser_cleProto_t* clePpIn = (cxa_protocolParser_cleProto_t*)superIn;
	cxa_assert(clePpIn);

	return (cxa_stateMachine_getCurrentState(&clePpIn->stateMachine) == RX_STATE_ERROR);
}


static bool scm_canSetBuffer(cxa_protocolParser_t *const superIn)
{
	cxa_protocolParser_cleProto_t* clePpIn = (cxa_protocolParser_cleProto_t*)superIn;
	cxa_assert(clePpIn);

	rxState_t currState = (rxState_t)cxa_stateMachine_getCurrentState(&clePpIn->stateMachine);
	return (currState == RX_STATE_PROCESS_PACKET) || (currState == RX_STATE_IDLE);
}


static void scm_gotoIdle(cxa_protocolParser_t *const superIn)
{
	cxa_protocolParser_cleProto_t* clePpIn = (cxa_protocolParser_cleProto_t*)superIn;
	cxa_assert(clePpIn);

	cxa_stateMachine_transitionNow(&clePpIn->stateMachine, RX_STATE_IDLE);
	cxa_assert( cxa_stateMachine_getCurrentState(&clePpIn->stateMachine) == RX_STATE_IDLE );
}


static bool scm_writeBytes(cxa_protocolParser_t *const superIn, cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_protocolParser_cleProto_t* clePpIn = (cxa_protocolParser_cleProto_t*)superIn;
	cxa_assert(clePpIn);

	// message _should_ be configured properly...get our size
	size_t msgSize_bytes = (fbbIn != NULL) ? cxa_fixedByteBuffer_getSize_bytes(fbbIn) : 0;
	cxa_assert(msgSize_bytes <= (65535-3));

	// make sure we're in a good state
	if( clePpIn->super.scm_isInError(&clePpIn->super) || !cxa_ioStream_isBound(clePpIn->super.ioStream) ) return false;

	// write our header
	if( !cxa_ioStream_writeByte(clePpIn->super.ioStream, 0x80) ) return false;
	if( !cxa_ioStream_writeByte(clePpIn->super.ioStream, 0x81) ) return false;

	size_t len = msgSize_bytes + 1;
	if( !cxa_ioStream_writeByte(clePpIn->super.ioStream, ((len & 0x00FF) >> 0)) ) return false;
	if( !cxa_ioStream_writeByte(clePpIn->super.ioStream, ((len & 0xFF00) >> 8)) ) return false;

	// write our data
	if( (fbbIn != NULL) && !cxa_ioStream_writeFixedByteBuffer(clePpIn->super.ioStream, fbbIn) ) return false;

	// write our footer
	if( !cxa_ioStream_writeByte(clePpIn->super.ioStream, 0x82) ) return false;

	return true;
}


static void rxState_cb_idle_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_protocolParser_cleProto_t* clePpIn = (cxa_protocolParser_cleProto_t*)userVarIn;
	cxa_assert(clePpIn);

	cxa_logger_info(&clePpIn->super.logger, "becoming idle");
}


static void rxState_cb_idle_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_cleProto_t* clePpIn = (cxa_protocolParser_cleProto_t*)userVarIn;
	cxa_assert(clePpIn);

	// if we have a bound ioStream and a buffer, become active
	if( cxa_ioStream_isBound(clePpIn->super.ioStream) && (clePpIn->super.currBuffer != NULL) )
	{
		cxa_stateMachine_transition(&clePpIn->stateMachine, RX_STATE_WAIT_0x80);
		return;
	}
}


static void rxState_cb_idle_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	cxa_protocolParser_cleProto_t* clePpIn = (cxa_protocolParser_cleProto_t*)userVarIn;
	cxa_assert(clePpIn);

	cxa_logger_info(&clePpIn->super.logger, "becoming active");
}


static void rxState_cb_wait0x80_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_cleProto_t* clePpIn = (cxa_protocolParser_cleProto_t*)userVarIn;
	cxa_assert(clePpIn);

	uint8_t rxByte;
	for( uint8_t i = 0; i < MAX_NUM_RX_BYTES_PER_UPDATE; i++ )
	{
		cxa_ioStream_readStatus_t readStat = cxa_ioStream_readByte(clePpIn->super.ioStream, &rxByte);
		if( readStat == CXA_IOSTREAM_READSTAT_ERROR ) { cxa_stateMachine_transition(&clePpIn->stateMachine, RX_STATE_ERROR); return; }
		else if( readStat == CXA_IOSTREAM_READSTAT_GOTDATA )
		{
			if( rxByte == 0x80 )
			{
				// we've gotten our first header byte
				cxa_fixedByteBuffer_clear(clePpIn->super.currBuffer);
				cxa_fixedByteBuffer_append_uint8(clePpIn->super.currBuffer, rxByte);

				// start our reception timeout timeDiff
				cxa_timeDiff_setStartTime_now(&clePpIn->super.td_timeout);

				cxa_stateMachine_transition(&clePpIn->stateMachine, RX_STATE_WAIT_0x81);
				return;
			}
		}
	}
}


static void rxState_cb_wait0x81_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_cleProto_t* clePpIn = (cxa_protocolParser_cleProto_t*)userVarIn;
	cxa_assert(clePpIn);

	uint8_t rxByte;
	cxa_ioStream_readStatus_t readStat = cxa_ioStream_readByte(clePpIn->super.ioStream, &rxByte);
	if( readStat == CXA_IOSTREAM_READSTAT_ERROR ) { cxa_stateMachine_transition(&clePpIn->stateMachine, RX_STATE_ERROR); return; }
	else if( readStat == CXA_IOSTREAM_READSTAT_GOTDATA )
	{
		// reset our reception timeout timeDiff
		cxa_timeDiff_setStartTime_now(&clePpIn->super.td_timeout);

		if( rxByte == 0x81 )
		{
			// we have a valid second header byte
			cxa_fixedByteBuffer_append_uint8(clePpIn->super.currBuffer, rxByte);
			cxa_stateMachine_transition(&clePpIn->stateMachine, RX_STATE_WAIT_LEN);
			return;
		}
		else
		{
			// we have an invalid second header byte
			cxa_stateMachine_transition(&clePpIn->stateMachine, RX_STATE_WAIT_0x80);
			return;
		}
	}

	// check to see if we've had a reception timeout
	if( cxa_timeDiff_isElapsed_ms(&clePpIn->super.td_timeout, RECEPTION_TIMEOUT_MS) )
	{
		cxa_protocolParser_notify_receptionTimeout(&clePpIn->super);
		cxa_stateMachine_transition(&clePpIn->stateMachine, RX_STATE_WAIT_0x80);
		return;
	}
}


static void rxState_cb_waitLen_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_cleProto_t* clePpIn = (cxa_protocolParser_cleProto_t*)userVarIn;
	cxa_assert(clePpIn);

	uint8_t rxByte;
	cxa_ioStream_readStatus_t readStat = cxa_ioStream_readByte(clePpIn->super.ioStream, &rxByte);
	if( readStat == CXA_IOSTREAM_READSTAT_ERROR ) { cxa_stateMachine_transition(&clePpIn->stateMachine, RX_STATE_ERROR); return; }
	else if( readStat == CXA_IOSTREAM_READSTAT_GOTDATA )
	{
		// reset our reception timeout timeDiff
		cxa_timeDiff_setStartTime_now(&clePpIn->super.td_timeout);

		// just append the byte
		cxa_fixedByteBuffer_append_uint8(clePpIn->super.currBuffer, rxByte);
		if( cxa_fixedByteBuffer_getSize_bytes(clePpIn->super.currBuffer) == 4 )
		{
			// we have all of our length bytes...make sure it's valid
			uint16_t len_bytes;
			if( cxa_fixedByteBuffer_get_uint16LE(clePpIn->super.currBuffer, 2, len_bytes) && (len_bytes >= 1) ) cxa_stateMachine_transition(&clePpIn->stateMachine, RX_STATE_WAIT_DATA_BYTES);
			return;
		}
	}

	// check to see if we've had a reception timeout
	if( cxa_timeDiff_isElapsed_ms(&clePpIn->super.td_timeout, RECEPTION_TIMEOUT_MS) )
	{
			cxa_protocolParser_notify_receptionTimeout(&clePpIn->super);
			cxa_stateMachine_transition(&clePpIn->stateMachine, RX_STATE_WAIT_0x80);
			return;
	}
}


static void rxState_cb_waitDataBytes_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_cleProto_t* clePpIn = (cxa_protocolParser_cleProto_t*)userVarIn;
	cxa_assert(clePpIn);

	// get our expected size
	uint16_t expectedSize_bytes;
	if( !cxa_fixedByteBuffer_get_uint16LE(clePpIn->super.currBuffer, 2, expectedSize_bytes) )
	{
		cxa_stateMachine_transition(&clePpIn->stateMachine, RX_STATE_WAIT_0x80);
		return;
	}

	// do a limited number of iterations
	for( uint8_t i = 0; i < MAX_NUM_RX_BYTES_PER_UPDATE; i++ )
	{
		size_t currSize_bytes = cxa_fixedByteBuffer_getSize_bytes(clePpIn->super.currBuffer) - 4;

		if( currSize_bytes < expectedSize_bytes )
		{
			// we have more bytes to receive
			uint8_t rxByte;
			cxa_ioStream_readStatus_t readStat = cxa_ioStream_readByte(clePpIn->super.ioStream, &rxByte);
			if( readStat == CXA_IOSTREAM_READSTAT_ERROR ) { cxa_stateMachine_transition(&clePpIn->stateMachine, RX_STATE_ERROR); return; }
			else if( readStat == CXA_IOSTREAM_READSTAT_GOTDATA )
			{
				// reset our reception timeout timeDiff
				cxa_timeDiff_setStartTime_now(&clePpIn->super.td_timeout);

				cxa_fixedByteBuffer_append_uint8(clePpIn->super.currBuffer, rxByte);
			}
		}
		else
		{
			// we're done receiving our data bytes
			cxa_stateMachine_transition(&clePpIn->stateMachine, RX_STATE_PROCESS_PACKET);
			return;
		}
	}

	// check to see if we've had a reception timeout
	if( cxa_timeDiff_isElapsed_ms(&clePpIn->super.td_timeout, RECEPTION_TIMEOUT_MS) )
	{
			cxa_protocolParser_notify_receptionTimeout(&clePpIn->super);
			cxa_stateMachine_transition(&clePpIn->stateMachine, RX_STATE_WAIT_0x80);
			return;
	}
}


static void rxState_cb_processPacket_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_cleProto_t* clePpIn = (cxa_protocolParser_cleProto_t*)userVarIn;
	cxa_assert(clePpIn);

	size_t currSize_bytes = cxa_fixedByteBuffer_getSize_bytes(clePpIn->super.currBuffer);

	uint8_t tmpVal8;
	uint16_t tmpVal16;

	// make sure our packet is kosher
	if( (currSize_bytes >= 5) &&
		(cxa_fixedByteBuffer_get_uint8(clePpIn->super.currBuffer, 0, tmpVal8) && (tmpVal8 == 0x80)) &&
		(cxa_fixedByteBuffer_get_uint8(clePpIn->super.currBuffer, 1, tmpVal8) && (tmpVal8 == 0x81)) &&
		(cxa_fixedByteBuffer_get_uint16LE(clePpIn->super.currBuffer, 2, tmpVal16) && (tmpVal16 == (currSize_bytes-4))) &&
		(cxa_fixedByteBuffer_get_uint8(clePpIn->super.currBuffer, currSize_bytes-1, tmpVal8) && (tmpVal8 == 0x82)) )
	{
		// we received a message
		cxa_logger_trace(&clePpIn->super.logger, "message received...calling listeners");

		// ...but first, strip the header and footer
		cxa_fixedByteBuffer_remove(clePpIn->super.currBuffer, 0, 4);
		cxa_fixedByteBuffer_remove(clePpIn->super.currBuffer, cxa_fixedByteBuffer_getSize_bytes(clePpIn->super.currBuffer)-1, 1);

		cxa_protocolParser_notify_packetReceived(&clePpIn->super, clePpIn->super.currBuffer);
	}
	else
	{
		cxa_logger_debug(&clePpIn->super.logger, "improperly formatted message received");
	}

	// no matter what, we'll reset and wait for more data
	cxa_stateMachine_transition(&clePpIn->stateMachine, RX_STATE_WAIT_0x80);
}


static void rxState_cb_error_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_protocolParser_cleProto_t* clePpIn = (cxa_protocolParser_cleProto_t*)userVarIn;
	cxa_assert(clePpIn);

	cxa_protocolParser_notify_ioException(&clePpIn->super);
}
