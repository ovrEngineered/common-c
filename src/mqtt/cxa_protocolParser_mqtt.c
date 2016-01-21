/**
 * Copyright 2015 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Christopher Armenio
 */
#include "cxa_protocolParser_mqtt.h"


// ******** includes ********
#include <string.h>
#include <cxa_assert.h>
#include <cxa_mqtt_message.h>
#include <cxa_mqtt_messageFactory.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_INFO
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define RECEPTION_TIMEOUT_MS		5000

#define ERR_FBB_OVERFLOW			"fbb overflow"
#define ERR_MALFORMED_PACKET		"malformed packet"
#define ERR_MALFORMED_HEADER		"malformed header"
#define ERR_INTERBYTE_TIMEOUT		"inter-byte timeout"


// ******** local type definitions ********
typedef enum
{
	RX_STATE_IDLE,
	RX_STATE_WAIT_FIXEDHEADER_1,
	RX_STATE_WAIT_REMAINING_LEN,
	RX_STATE_WAIT_DATABYTES,
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
static void rxStateCb_waitFixedHeader1_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxStateCb_waitRemainingLen_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxStateCb_waitDataBytes_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxStateCb_processPacket_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void rxState_cb_error_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_protocolParser_mqtt_init(cxa_protocolParser_mqtt_t *const mppIn, cxa_ioStream_t *const ioStreamIn, cxa_fixedByteBuffer_t *const buffIn)
{
	cxa_assert(mppIn);
	cxa_assert(ioStreamIn);

	// initialize our super class
	cxa_protocolParser_init(&mppIn->super, ioStreamIn, buffIn, scm_isInErrorState, scm_canSetBuffer, scm_gotoIdle, scm_writeBytes);

	// set some default values
	mppIn->remainingBytesToReceive = 0;

	// setup our state machine
	cxa_stateMachine_init(&mppIn->stateMachine, "mqttProtoParser");
	cxa_stateMachine_addState(&mppIn->stateMachine, RX_STATE_IDLE, "idle", rxState_cb_idle_enter, rxState_cb_idle_state, rxState_cb_idle_leave, (void*)mppIn);
	cxa_stateMachine_addState(&mppIn->stateMachine, RX_STATE_WAIT_FIXEDHEADER_1, "wait_fh1", NULL, rxStateCb_waitFixedHeader1_state, NULL, (void*)mppIn);
	cxa_stateMachine_addState(&mppIn->stateMachine, RX_STATE_WAIT_REMAINING_LEN, "wait_remLen", NULL, rxStateCb_waitRemainingLen_state, NULL, (void*)mppIn);
	cxa_stateMachine_addState(&mppIn->stateMachine, RX_STATE_WAIT_DATABYTES, "wait_dataBytes", NULL, rxStateCb_waitDataBytes_state, NULL, (void*)mppIn);
	cxa_stateMachine_addState(&mppIn->stateMachine, RX_STATE_PROCESS_PACKET, "processPacket", rxStateCb_processPacket_enter, NULL, NULL, (void*)mppIn);
	cxa_stateMachine_addState(&mppIn->stateMachine, RX_STATE_ERROR, "error", rxState_cb_error_enter, NULL, NULL, (void*)mppIn);
	cxa_stateMachine_transition(&mppIn->stateMachine, RX_STATE_IDLE);
	cxa_stateMachine_update(&mppIn->stateMachine);
}


void cxa_protocolParser_mqtt_update(cxa_protocolParser_mqtt_t *const mppIn)
{
	cxa_assert(mppIn);

	cxa_stateMachine_update(&mppIn->stateMachine);
}


// ******** local function implementations ********
static bool scm_isInErrorState(cxa_protocolParser_t *const superIn)
{
	cxa_protocolParser_mqtt_t* mppIn = (cxa_protocolParser_mqtt_t*)superIn;
	cxa_assert(mppIn);

	return (cxa_stateMachine_getCurrentState(&mppIn->stateMachine) == RX_STATE_ERROR);
}


static bool scm_canSetBuffer(cxa_protocolParser_t *const superIn)
{
	cxa_protocolParser_mqtt_t* mppIn = (cxa_protocolParser_mqtt_t*)superIn;
	cxa_assert(mppIn);

	rxState_t currState = cxa_stateMachine_getCurrentState(&mppIn->stateMachine);
	return (currState == RX_STATE_PROCESS_PACKET) || (currState == RX_STATE_IDLE);
}


static void scm_gotoIdle(cxa_protocolParser_t *const superIn)
{
	cxa_protocolParser_mqtt_t* mppIn = (cxa_protocolParser_mqtt_t*)superIn;
	cxa_assert(mppIn);

	cxa_stateMachine_transition(&mppIn->stateMachine, RX_STATE_IDLE);
	cxa_protocolParser_mqtt_update(mppIn);
	cxa_assert( cxa_stateMachine_getCurrentState(&mppIn->stateMachine) == RX_STATE_IDLE );
}


static bool scm_writeBytes(cxa_protocolParser_t *const superIn, cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_protocolParser_mqtt_t* mppIn = (cxa_protocolParser_mqtt_t*)superIn;
	cxa_assert(mppIn);

	cxa_mqtt_message_t* msg = cxa_mqtt_messageFactory_getMessage_byBuffer(fbbIn);
	if( msg == NULL ) return false;

	// ensure our length field is up-to-date
	if( !cxa_mqtt_message_updateVariableLengthField(msg) ) return false;

	// write it!
	return cxa_ioStream_writeFixedByteBuffer(mppIn->super.ioStream, fbbIn);
}


static void rxState_cb_idle_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_protocolParser_mqtt_t* mppIn = (cxa_protocolParser_mqtt_t*)userVarIn;
	cxa_assert(mppIn);

	cxa_logger_info(&mppIn->super.logger, "becoming idle");
}


static void rxState_cb_idle_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_mqtt_t* mppIn = (cxa_protocolParser_mqtt_t*)userVarIn;
	cxa_assert(mppIn);

	// if we have a bound ioStream and a buffer, become active
	if( cxa_ioStream_isBound(mppIn->super.ioStream) && (mppIn->super.currBuffer != NULL) )
	{
		cxa_stateMachine_transition(&mppIn->stateMachine, RX_STATE_WAIT_FIXEDHEADER_1);
		return;
	}
}


static void rxState_cb_idle_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	cxa_protocolParser_mqtt_t* mppIn = (cxa_protocolParser_mqtt_t*)userVarIn;
	cxa_assert(mppIn);

	cxa_logger_info(&mppIn->super.logger, "becoming active");
}


static void rxStateCb_waitFixedHeader1_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_mqtt_t *mppIn = (cxa_protocolParser_mqtt_t*)userVarIn;
	cxa_assert(mppIn);

	uint8_t rxByte;
	if( cxa_ioStream_readByte(mppIn->super.ioStream, &rxByte) == CXA_IOSTREAM_READSTAT_GOTDATA )
	{
		bool doFlagsMatch = false;
		switch( cxa_mqtt_message_rxBytes_getType(rxByte) )
		{
			case CXA_MQTT_MSGTYPE_CONNECT:
			case CXA_MQTT_MSGTYPE_CONNACK:
			case CXA_MQTT_MSGTYPE_PINGREQ:
			case CXA_MQTT_MSGTYPE_PINGRESP:
			case CXA_MQTT_MSGTYPE_SUBACK:
				// make sure the flags match
				doFlagsMatch = (rxByte & 0x0F) == 0;
				break;

			case CXA_MQTT_MSGTYPE_SUBSCRIBE:
				// make sure the flags match
				doFlagsMatch = (rxByte & 0x0F) == 0x02;
				break;

			case CXA_MQTT_MSGTYPE_PUBLISH:
				// flags don't matter for this one (can be anything)
				doFlagsMatch = true;
				break;

			default:
				cxa_logger_warn(&mppIn->super.logger, "unknown header byte: 0x%02X", rxByte);
				return;
		}

		// if we made it here, we at least know what kind of packet this is...
		if( doFlagsMatch )
		{
			// clear our buffer and add the first byte
			cxa_fixedByteBuffer_clear(mppIn->super.currBuffer);

			if( cxa_fixedByteBuffer_append_uint8(mppIn->super.currBuffer, rxByte) )
			{
				// start our reception timeout timeDiff
				cxa_timeDiff_setStartTime_now(&mppIn->super.td_timeout);

				cxa_stateMachine_transition(&mppIn->stateMachine, RX_STATE_WAIT_REMAINING_LEN);
				return;
			}
			else cxa_logger_warn(&mppIn->super.logger, ERR_FBB_OVERFLOW);
		} else cxa_logger_warn(&mppIn->super.logger, ERR_MALFORMED_HEADER);
	}
}


static void rxStateCb_waitRemainingLen_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_mqtt_t *mppIn = (cxa_protocolParser_mqtt_t*)userVarIn;
	cxa_assert(mppIn);

	uint8_t rxByte;
	if( cxa_ioStream_readByte(mppIn->super.ioStream, &rxByte) == CXA_IOSTREAM_READSTAT_GOTDATA )
	{
		// reset our reception timeout timeDiff
		cxa_timeDiff_setStartTime_now(&mppIn->super.td_timeout);

		// add to our buffer
		if( !cxa_fixedByteBuffer_append_uint8(mppIn->super.currBuffer, rxByte) )
		{
			cxa_logger_warn(&mppIn->super.logger, ERR_FBB_OVERFLOW);
			cxa_stateMachine_transition(&mppIn->stateMachine, RX_STATE_WAIT_FIXEDHEADER_1);
			return;
		}

		// process our variable length field (or the fraction we currently have)
		bool isVarLengthComplete;
		size_t actualLength;
		if( !cxa_mqtt_message_rxBytes_parseVariableLengthField(mppIn->super.currBuffer, &isVarLengthComplete, &actualLength, NULL) )
		{
			cxa_logger_warn(&mppIn->super.logger, ERR_MALFORMED_HEADER);
			cxa_stateMachine_transition(&mppIn->stateMachine, RX_STATE_WAIT_FIXEDHEADER_1);
			return;
		}

		if( isVarLengthComplete )
		{
			mppIn->remainingBytesToReceive = actualLength;
			cxa_logger_trace(&mppIn->super.logger, "waiting for %d bytes", mppIn->remainingBytesToReceive);
			cxa_stateMachine_transition(&mppIn->stateMachine, RX_STATE_WAIT_DATABYTES);
			return;
		}
	}

	// check to see if we've had a reception timeout
	if( cxa_timeDiff_isElapsed_ms(&mppIn->super.td_timeout, RECEPTION_TIMEOUT_MS) )
	{
		cxa_protocolParser_notify_receptionTimeout(&mppIn->super);
		cxa_stateMachine_transition(&mppIn->stateMachine, RX_STATE_WAIT_FIXEDHEADER_1);
		return;
	}
}


static void rxStateCb_waitDataBytes_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_mqtt_t *mppIn = (cxa_protocolParser_mqtt_t*)userVarIn;
	cxa_assert(mppIn);

	// see if we've gotten enough bytes yet...
	if( mppIn->remainingBytesToReceive == 0 )
	{
		cxa_stateMachine_transition(&mppIn->stateMachine, RX_STATE_PROCESS_PACKET);
		return;
	}

	// keep receiving bytes
	uint8_t rxByte;
	if( cxa_ioStream_readByte(mppIn->super.ioStream, &rxByte) == CXA_IOSTREAM_READSTAT_GOTDATA )
	{
		// reset our reception timeout timeDiff
		cxa_timeDiff_setStartTime_now(&mppIn->super.td_timeout);

		// add to our buffer
		if( !cxa_fixedByteBuffer_append_uint8(mppIn->super.currBuffer, rxByte) )
		{
			cxa_logger_warn(&mppIn->super.logger, ERR_FBB_OVERFLOW);
			cxa_stateMachine_transition(&mppIn->stateMachine, RX_STATE_WAIT_FIXEDHEADER_1);
			return;
		}
		mppIn->remainingBytesToReceive--;
	}

	// check to see if we've had a reception timeout
	if( cxa_timeDiff_isElapsed_ms(&mppIn->super.td_timeout, RECEPTION_TIMEOUT_MS) )
	{
		cxa_protocolParser_notify_receptionTimeout(&mppIn->super);
		cxa_stateMachine_transition(&mppIn->stateMachine, RX_STATE_WAIT_FIXEDHEADER_1);
		return;
	}
}


static void rxStateCb_processPacket_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn,void *userVarIn)
{
	cxa_protocolParser_mqtt_t *mppIn = (cxa_protocolParser_mqtt_t*)userVarIn;
	cxa_assert(mppIn);

	// make sure our packet is kosher
	cxa_mqtt_message_t* msg = cxa_mqtt_messageFactory_getMessage_byBuffer(mppIn->super.currBuffer);
	if( (msg != NULL) && cxa_mqtt_message_validateReceivedBytes(msg) )
	{
		// we received a message
		cxa_logger_trace(&mppIn->super.logger, "message received...calling listeners");

		cxa_protocolParser_notify_packetReceived(&mppIn->super, mppIn->super.currBuffer);
	}
	else cxa_logger_debug(&mppIn->super.logger, ERR_MALFORMED_PACKET);

	// no matter what, we'll reset and wait for more data
	cxa_stateMachine_transition(&mppIn->stateMachine, RX_STATE_WAIT_FIXEDHEADER_1);
	return;
}


static void rxState_cb_error_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_protocolParser_mqtt_t* mppIn = (cxa_protocolParser_mqtt_t*)userVarIn;
	cxa_assert(mppIn);

	cxa_protocolParser_notify_ioException(&mppIn->super);
}
