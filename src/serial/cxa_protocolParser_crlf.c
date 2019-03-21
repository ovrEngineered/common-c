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
#include "cxa_protocolParser_crlf.h"


// ******** includes ********
#include <cxa_assert.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_DEBUG
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define MAX_NUM_RX_BYTES_PER_UPDATE		16
#define RECEPTION_TIMEOUT_MS			5000


// ******** local type definitions ********
typedef enum
{
	RX_STATE_IDLE,
	RX_STATE_WAIT_FIRSTBYTE,
	RX_STATE_WAIT_CR,
	RX_STATE_WAIT_LF,
	RX_STATE_PROCESS_PACKET,
	RX_STATE_ERROR
}rxState_t;


// ******** local function prototypes ********
static bool scm_isInErrorState(cxa_protocolParser_t *const superIn);
static bool scm_canSetBuffer(cxa_protocolParser_t *const superIn);
static void scm_gotoIdle(cxa_protocolParser_t *const superIn);
static void scm_reset(cxa_protocolParser_t *const superIn);
static bool scm_writeBytes(cxa_protocolParser_t *const superIn, cxa_fixedByteBuffer_t *const fbbIn);


static void rxState_cb_idle_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void rxState_cb_idle_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_idle_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void rxState_cb_waitFirstByte_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_waitCrLf_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_processPacket_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_error_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_protocolParser_crlf_init(cxa_protocolParser_crlf_t *const crlfPpIn, cxa_ioStream_t *const ioStreamIn, cxa_fixedByteBuffer_t *const buffIn, int threadIdIn)
{
	cxa_assert(crlfPpIn);
	cxa_assert(ioStreamIn);

	// set our initial state
	crlfPpIn->isPaused = true;

	// initialize our super class
	cxa_protocolParser_init(&crlfPpIn->super, ioStreamIn, buffIn, scm_isInErrorState, scm_canSetBuffer, scm_gotoIdle, scm_reset, scm_writeBytes);

	// setup our state machine
	cxa_stateMachine_init(&crlfPpIn->stateMachine, "crlfParser", threadIdIn);
	cxa_stateMachine_addState(&crlfPpIn->stateMachine, RX_STATE_IDLE, "idle", rxState_cb_idle_enter, rxState_cb_idle_state, rxState_cb_idle_leave, (void*)crlfPpIn);
	cxa_stateMachine_addState(&crlfPpIn->stateMachine, RX_STATE_WAIT_FIRSTBYTE, "wait_firstByte", NULL, rxState_cb_waitFirstByte_state, NULL, (void*)crlfPpIn);
	cxa_stateMachine_addState(&crlfPpIn->stateMachine, RX_STATE_WAIT_CR, "wait_CR", NULL, rxState_cb_waitCrLf_state, NULL, (void*)crlfPpIn);
	cxa_stateMachine_addState(&crlfPpIn->stateMachine, RX_STATE_WAIT_LF, "wait_LF", NULL, rxState_cb_waitCrLf_state, NULL, (void*)crlfPpIn);
	cxa_stateMachine_addState(&crlfPpIn->stateMachine, RX_STATE_PROCESS_PACKET, "processPacket", NULL, rxState_cb_processPacket_state, NULL, (void*)crlfPpIn);
	cxa_stateMachine_addState(&crlfPpIn->stateMachine, RX_STATE_ERROR, "error", rxState_cb_error_enter, NULL, NULL, (void*)crlfPpIn);
	cxa_stateMachine_setInitialState(&crlfPpIn->stateMachine, RX_STATE_IDLE);
}


void cxa_protocolParser_crlf_pause(cxa_protocolParser_crlf_t *const crlfPpIn)
{
	cxa_assert(crlfPpIn);

	crlfPpIn->isPaused = true;
	cxa_stateMachine_transition(&crlfPpIn->stateMachine, RX_STATE_IDLE);
}


void cxa_protocolParser_crlf_resume(cxa_protocolParser_crlf_t *const crlfPpIn)
{
	cxa_assert(crlfPpIn);

	crlfPpIn->isPaused = false;
	// will automatically transition out of idle if needed
	cxa_stateMachine_transition(&crlfPpIn->stateMachine, RX_STATE_IDLE);
}


// ******** local function implementations ********
static bool scm_isInErrorState(cxa_protocolParser_t *const superIn)
{
	cxa_protocolParser_crlf_t* crlfPpIn = (cxa_protocolParser_crlf_t*)superIn;
	cxa_assert(crlfPpIn);

	return (cxa_stateMachine_getCurrentState(&crlfPpIn->stateMachine) == RX_STATE_ERROR);
}


static bool scm_canSetBuffer(cxa_protocolParser_t *const superIn)
{
	cxa_protocolParser_crlf_t* crlfPpIn = (cxa_protocolParser_crlf_t*)superIn;
	cxa_assert(crlfPpIn);

	rxState_t currState = (rxState_t)cxa_stateMachine_getCurrentState(&crlfPpIn->stateMachine);
	return (currState == RX_STATE_PROCESS_PACKET) || (currState == RX_STATE_IDLE);
}


static void scm_gotoIdle(cxa_protocolParser_t *const superIn)
{
	cxa_protocolParser_crlf_t* crlfPpIn = (cxa_protocolParser_crlf_t*)superIn;
	cxa_assert(crlfPpIn);

	cxa_stateMachine_transitionNow(&crlfPpIn->stateMachine, RX_STATE_IDLE);
	cxa_assert( cxa_stateMachine_getCurrentState(&crlfPpIn->stateMachine) == RX_STATE_IDLE );
}


static void scm_reset(cxa_protocolParser_t *const superIn)
{
	cxa_protocolParser_crlf_t* crlfPpIn = (cxa_protocolParser_crlf_t*)superIn;
	cxa_assert(crlfPpIn);

	rxState_t currState = (rxState_t)cxa_stateMachine_getCurrentState(&crlfPpIn->stateMachine);
	if( (currState != RX_STATE_IDLE) && (currState != RX_STATE_ERROR) )
	{
		cxa_stateMachine_transitionNow(&crlfPpIn->stateMachine, RX_STATE_WAIT_FIRSTBYTE);
	}
}


static bool scm_writeBytes(cxa_protocolParser_t *const superIn, cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_protocolParser_crlf_t* crlfPpIn = (cxa_protocolParser_crlf_t*)superIn;
	cxa_assert(crlfPpIn);

	// write our data
	if( (fbbIn != NULL) && !cxa_ioStream_writeFixedByteBuffer(crlfPpIn->super.ioStream, fbbIn) ) return false;

	// write our CRLF
	if( !cxa_ioStream_writeString(crlfPpIn->super.ioStream, "\r\n") ) return false;

	return true;
}


static void rxState_cb_idle_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_protocolParser_crlf_t* crlfPpIn = (cxa_protocolParser_crlf_t*)userVarIn;
	cxa_assert(crlfPpIn);

	cxa_logger_info(&crlfPpIn->super.logger, "becoming idle");
}


static void rxState_cb_idle_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_crlf_t* crlfPpIn = (cxa_protocolParser_crlf_t*)userVarIn;
	cxa_assert(crlfPpIn);

	// if we have a bound ioStream and a buffer, become active
	if( cxa_ioStream_isBound(crlfPpIn->super.ioStream) && (crlfPpIn->super.currBuffer != NULL) && !crlfPpIn->isPaused )
	{
		cxa_stateMachine_transition(&crlfPpIn->stateMachine, RX_STATE_WAIT_FIRSTBYTE);
		return;
	}
}


static void rxState_cb_idle_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	cxa_protocolParser_crlf_t* crlfPpIn = (cxa_protocolParser_crlf_t*)userVarIn;
	cxa_assert(crlfPpIn);

	cxa_logger_info(&crlfPpIn->super.logger, "becoming active");
}


static void rxState_cb_waitFirstByte_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_crlf_t* crlfPpIn = (cxa_protocolParser_crlf_t*)userVarIn;
	cxa_assert(crlfPpIn);

	uint8_t rxByte;
	for( uint8_t i = 0; i < MAX_NUM_RX_BYTES_PER_UPDATE; i++ )
	{
		// make sure we haven't been paused
		if( crlfPpIn->isPaused ) return;

		cxa_ioStream_readStatus_t readStat = cxa_ioStream_readByte(crlfPpIn->super.ioStream, &rxByte);
		if( readStat == CXA_IOSTREAM_READSTAT_ERROR ) { cxa_stateMachine_transition(&crlfPpIn->stateMachine, RX_STATE_ERROR); return; }
		else if( readStat == CXA_IOSTREAM_READSTAT_GOTDATA )
		{
			// we've gotten our first byte
			cxa_fixedByteBuffer_clear(crlfPpIn->super.currBuffer);
			cxa_fixedByteBuffer_append_uint8(crlfPpIn->super.currBuffer, rxByte);

			// reset our reception timeout timeDiff
			cxa_timeDiff_setStartTime_now(&crlfPpIn->super.td_timeout);

			// next state depends on the byte
			if( rxByte == '\r' )
			{
				cxa_stateMachine_transition(&crlfPpIn->stateMachine, RX_STATE_WAIT_LF);
			}
			else
			{
				cxa_stateMachine_transition(&crlfPpIn->stateMachine, RX_STATE_WAIT_CR);
			}
			return;
		}
	}
}


static void rxState_cb_waitCrLf_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_crlf_t* crlfPpIn = (cxa_protocolParser_crlf_t*)userVarIn;
	cxa_assert(crlfPpIn);

	uint8_t rxByte;
	for( uint8_t i = 0; i < MAX_NUM_RX_BYTES_PER_UPDATE; i++ )
	{
		// make sure we haven't been paused
		if( crlfPpIn->isPaused ) return;

		cxa_ioStream_readStatus_t readStat = cxa_ioStream_readByte(crlfPpIn->super.ioStream, &rxByte);
		if( readStat == CXA_IOSTREAM_READSTAT_ERROR ) { cxa_stateMachine_transition(&crlfPpIn->stateMachine, RX_STATE_ERROR); return; }
		else if( readStat == CXA_IOSTREAM_READSTAT_GOTDATA )
		{
			// reset our reception timeout timeDiff
			cxa_timeDiff_setStartTime_now(&crlfPpIn->super.td_timeout);

			// save the byte
			cxa_fixedByteBuffer_append_uint8(crlfPpIn->super.currBuffer, rxByte);

			// see if we got the next byte we're looking for
			rxState_t currState = cxa_stateMachine_getCurrentState(&crlfPpIn->stateMachine);
			uint8_t targetByte = (currState == RX_STATE_WAIT_LF) ? '\n' : '\r';

			if( rxByte == targetByte )
			{
				// this is the byte we're looking for
				if( currState == RX_STATE_WAIT_CR )
				{
					cxa_stateMachine_transition(&crlfPpIn->stateMachine, RX_STATE_WAIT_LF);
					return;
				}
				else if( currState == RX_STATE_WAIT_LF )
				{
					cxa_stateMachine_transition(smIn, RX_STATE_PROCESS_PACKET);
					return;
				}
			}
			else
			{
				// if we already moved on in the state machine, reset ourselves
				if( currState == RX_STATE_WAIT_LF )
				{
					cxa_stateMachine_transition(&crlfPpIn->stateMachine, RX_STATE_WAIT_CR);
					return;
				}
			}
		}

		// check to see if we've had a reception timeout
		if( cxa_timeDiff_isElapsed_ms(&crlfPpIn->super.td_timeout, RECEPTION_TIMEOUT_MS) )
		{
			cxa_logger_debug_memDump_fbb(&crlfPpIn->super.logger, "buff: ", crlfPpIn->super.currBuffer, NULL);
			cxa_protocolParser_notify_receptionTimeout(&crlfPpIn->super);
			cxa_stateMachine_transition(&crlfPpIn->stateMachine, RX_STATE_WAIT_FIRSTBYTE);
			return;
		}
	}
}


static void rxState_cb_processPacket_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_crlf_t* crlfPpIn = (cxa_protocolParser_crlf_t*)userVarIn;
	cxa_assert(crlfPpIn);

	size_t currSize_bytes = cxa_fixedByteBuffer_getSize_bytes(crlfPpIn->super.currBuffer);

	uint8_t tmpVal8;

	// make sure our packet is kosher
	if( (currSize_bytes >= 2) &&
		(cxa_fixedByteBuffer_get_uint8(crlfPpIn->super.currBuffer, currSize_bytes-2, tmpVal8) && (tmpVal8 == '\r')) &&
		(cxa_fixedByteBuffer_get_uint8(crlfPpIn->super.currBuffer, currSize_bytes-1, tmpVal8) && (tmpVal8 == '\n')) )
	{
		// we received a message
		cxa_logger_trace(&crlfPpIn->super.logger, "message received...calling listeners");

		// ...but first, strip the crlf and null terminate
		cxa_fixedByteBuffer_replace_uint8(crlfPpIn->super.currBuffer, currSize_bytes-2, 0);
		cxa_fixedByteBuffer_remove(crlfPpIn->super.currBuffer, currSize_bytes-1, 1);

		cxa_protocolParser_notify_packetReceived(&crlfPpIn->super, crlfPpIn->super.currBuffer);
	}
	else
	{
		cxa_logger_debug(&crlfPpIn->super.logger, "improperly formatted message received");
	}

	// no matter what, we'll reset and wait for more data
	cxa_stateMachine_transition(&crlfPpIn->stateMachine, RX_STATE_WAIT_FIRSTBYTE);
}


static void rxState_cb_error_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_protocolParser_crlf_t* crlfPpIn = (cxa_protocolParser_crlf_t*)userVarIn;
	cxa_assert(crlfPpIn);

	cxa_protocolParser_notify_ioException(&crlfPpIn->super);
}
