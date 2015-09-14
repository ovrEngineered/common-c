/**
 * Copyright 2013 opencxa.org
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
 */
#include "cxa_rpc_protocolParser.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdio.h>
#include <cxa_assert.h>
#include <cxa_rpc_messageFactory.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_DEBUG
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define PROTOCOL_VERSION				1
#define MAX_NUM_RX_BYTES_PER_UPDATE		16


// ******** local type definitions ********
typedef enum
{
	RX_STATE_IDLE,
	RX_STATE_WAIT_0x80,
	RX_STATE_WAIT_0x81,
	RX_STATE_WAIT_LEN,
	RX_STATE_WAIT_DATA_BYTES,
	RX_STATE_PROCESS_MESSAGE,
	RX_STATE_ERROR
}rxState_t;


// ******** local function prototypes ********
static void handleIoException(cxa_rpc_protocolParser_t *const rppIn);
static void rxState_cb_idle_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_idle_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_idle_leave(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_wait0x80_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_wait0x81_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_waitLen_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_waitDataBytes_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_processMessage_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_error_enter(cxa_stateMachine_t *const smIn, void *userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_rpc_protocolParser_init(cxa_rpc_protocolParser_t *const rppIn, uint8_t userProtoVersionIn, cxa_ioStream_t *ioStreamIn)
{
	cxa_assert(rppIn);
	cxa_assert(userProtoVersionIn <= 15);
	cxa_assert(ioStreamIn);
	
	// save our references
	rppIn->userProtoVersion = userProtoVersionIn;
	rppIn->ioStream = ioStreamIn;
	rppIn->currRxMsg = NULL;

	// setup our logger
	cxa_logger_init(&rppIn->logger, "protocolParser");

	// setup our listeners
	cxa_array_initStd(&rppIn->protocolListeners, rppIn->protocolListeners_raw);
	cxa_array_initStd(&rppIn->messageListeners, rppIn->messageListeners_raw);

	// setup our state machine
	cxa_stateMachine_init(&rppIn->stateMachine, "protocolParser");
	cxa_stateMachine_addState(&rppIn->stateMachine, RX_STATE_IDLE, "idle", rxState_cb_idle_enter, rxState_cb_idle_state, rxState_cb_idle_leave, (void*)rppIn);
	cxa_stateMachine_addState(&rppIn->stateMachine, RX_STATE_WAIT_0x80, "wait_0x80", NULL, rxState_cb_wait0x80_state, NULL, (void*)rppIn);
	cxa_stateMachine_addState(&rppIn->stateMachine, RX_STATE_WAIT_0x81, "wait_0x81", NULL, rxState_cb_wait0x81_state, NULL, (void*)rppIn);
	cxa_stateMachine_addState(&rppIn->stateMachine, RX_STATE_WAIT_LEN, "wait_len", NULL, rxState_cb_waitLen_state, NULL, (void*)rppIn);
	cxa_stateMachine_addState(&rppIn->stateMachine, RX_STATE_WAIT_DATA_BYTES, "wait_dataBytes", NULL, rxState_cb_waitDataBytes_state, NULL, (void*)rppIn);
	cxa_stateMachine_addState(&rppIn->stateMachine, RX_STATE_PROCESS_MESSAGE, "processMsg", NULL, rxState_cb_processMessage_state, NULL, (void*)rppIn);
	cxa_stateMachine_addState(&rppIn->stateMachine, RX_STATE_ERROR, "error", rxState_cb_error_enter, NULL, NULL, (void*)rppIn);

	// set our initial state
	cxa_stateMachine_transition(&rppIn->stateMachine, RX_STATE_IDLE);
	cxa_stateMachine_update(&rppIn->stateMachine);
}


void cxa_rpc_protocolParser_deinit(cxa_rpc_protocolParser_t *const rppIn)
{
	cxa_assert(rppIn);

	// make sure we go idle
	if( cxa_stateMachine_getCurrentState(&rppIn->stateMachine) != RX_STATE_IDLE )
	{
		cxa_stateMachine_transition(&rppIn->stateMachine, RX_STATE_IDLE);
		cxa_stateMachine_update(&rppIn->stateMachine);
	}
}


bool cxa_rpc_protocolParser_writeMessage(cxa_rpc_protocolParser_t *const rppIn, cxa_rpc_message_t *const msgToWriteIn)
{
	cxa_assert(rppIn);
	cxa_assert(msgToWriteIn);

	// make sure the message is configured properly
	if( cxa_rpc_message_getType(msgToWriteIn) == CXA_RPC_MESSAGE_TYPE_UNKNOWN ) return false;

	// message _should_ be configured properly...get our size
	size_t msgSize_bytes = cxa_fixedByteBuffer_getSize_bytes(msgToWriteIn->buffer);
	cxa_assert(msgSize_bytes <= (65535-3));

	// make sure we're in a good state
	rxState_t currState = cxa_stateMachine_getCurrentState(&rppIn->stateMachine);
	if( (currState == RX_STATE_ERROR) || !cxa_ioStream_isBound(rppIn->ioStream) ) return false;

	// write our header
	if( !cxa_ioStream_writeByte(rppIn->ioStream, 0x80) ) { handleIoException(rppIn); return false; }
	if( !cxa_ioStream_writeByte(rppIn->ioStream, 0x81) ) { handleIoException(rppIn); return false; }

	size_t len = msgSize_bytes + 2;
	if( !cxa_ioStream_writeByte(rppIn->ioStream, ((len & 0x00FF) >> 0)) ) { handleIoException(rppIn); return false; }
	if( !cxa_ioStream_writeByte(rppIn->ioStream, ((len & 0xFF00) >> 8)) ) { handleIoException(rppIn); return false; }
	if( !cxa_ioStream_writeByte(rppIn->ioStream, ((PROTOCOL_VERSION << 4) | rppIn->userProtoVersion)) ) { handleIoException(rppIn); return false; }

	// write our data
	if( !cxa_ioStream_writeFixedByteBuffer(rppIn->ioStream, msgToWriteIn->buffer) ) { handleIoException(rppIn); return false; }
	
	// write our footer
	if( !cxa_ioStream_writeByte(rppIn->ioStream, 0x82) ) { handleIoException(rppIn); return false; }
	return true;
}


void cxa_rpc_protocolParser_addProtocolListener(cxa_rpc_protocolParser_t *const rppIn,
	cxa_rpc_protocolParser_cb_invalidVersionNumber_t cb_invalidVerIn,
	cxa_rpc_protocolParser_cb_ioExceptionOccurred_t cb_exceptionIn,
	void *const userVarIn)
{
	cxa_assert(rppIn);
	
	// create and add our new entry
	cxa_rpc_protocolParser_protoListener_entry_t newEntry = {.cb_invalidVer=cb_invalidVerIn, .cb_exception=cb_exceptionIn, .userVar=userVarIn};
	cxa_assert( cxa_array_append(&rppIn->protocolListeners, &newEntry) );
}


void cxa_rpc_protocolParser_addMessageListener(cxa_rpc_protocolParser_t *const rppIn,
	cxa_rpc_protocolParser_cb_messageReceived_t cb_msgRxIn,
	void *const userVarIn)
{
	cxa_assert(rppIn);
	
	// create and add our new entry
	cxa_rpc_protocolParser_messageListener_entry_t newEntry = {.cb=cb_msgRxIn, .userVar=userVarIn};
	cxa_assert( cxa_array_append(&rppIn->messageListeners, &newEntry) );
}


void cxa_rpc_protocolParser_update(cxa_rpc_protocolParser_t *const rppIn)
{
	cxa_assert(rppIn);
	
	cxa_stateMachine_update(&rppIn->stateMachine);
}


// ******** local function implementations ********
static void handleIoException(cxa_rpc_protocolParser_t *const rppIn)
{
	cxa_assert(rppIn);
	
	cxa_logger_error(&rppIn->logger, "exception occurred with underlying serial device");
	
	cxa_stateMachine_transition(&rppIn->stateMachine, RX_STATE_ERROR);
}


static void rxState_cb_idle_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_rpc_protocolParser_t *const rppIn = (cxa_rpc_protocolParser_t *const)userVarIn;
	cxa_assert(rppIn);
	
	cxa_logger_info(&rppIn->logger, "becoming idle");
	if( (rppIn->currRxMsg != NULL) && (cxa_rpc_messageFactory_getReferenceCountForMessage(rppIn->currRxMsg) > 0) )
	{
		cxa_rpc_messageFactory_decrementMessageRefCount(rppIn->currRxMsg);
	}
}


static void rxState_cb_idle_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_rpc_protocolParser_t *const rppIn = (cxa_rpc_protocolParser_t *const)userVarIn;
	cxa_assert(rppIn);

	// if we have a bound ioStream, become active
	if( cxa_ioStream_isBound(rppIn->ioStream) )
	{
		cxa_stateMachine_transition(&rppIn->stateMachine, RX_STATE_WAIT_0x80);
		return;
	}
}


static void rxState_cb_idle_leave(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_rpc_protocolParser_t *const rppIn = (cxa_rpc_protocolParser_t *const)userVarIn;
	cxa_assert(rppIn);
	
	// try to reserve a free RX buffer
	rppIn->currRxMsg = cxa_rpc_messageFactory_getFreeMessage_empty();
	if( rppIn->currRxMsg == NULL )
	{
		cxa_logger_warn(&rppIn->logger, "could not reserve free rx buffer");
		cxa_stateMachine_transition(&rppIn->stateMachine, RX_STATE_ERROR);
		cxa_stateMachine_update(&rppIn->stateMachine);
	}
	else cxa_logger_info(&rppIn->logger, "becoming active");
}


static void rxState_cb_wait0x80_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_rpc_protocolParser_t *const rppIn = (cxa_rpc_protocolParser_t *const)userVarIn;
	cxa_assert(rppIn);

	uint8_t rxByte;
	for( uint8_t i = 0; i < MAX_NUM_RX_BYTES_PER_UPDATE; i++ )
	{
		cxa_ioStream_readStatus_t readStat = cxa_ioStream_readByte(rppIn->ioStream, &rxByte);
		if( readStat == CXA_IOSTREAM_READSTAT_ERROR ) { handleIoException(rppIn); return; }
		else if( readStat == CXA_IOSTREAM_READSTAT_GOTDATA )
		{
			if( rxByte == 0x80 )
			{
				// we've gotten our first header byte
				cxa_fixedByteBuffer_clear(rppIn->currRxMsg->buffer);
				cxa_fixedByteBuffer_append_uint8(rppIn->currRxMsg->buffer, rxByte);

				cxa_stateMachine_transition(&rppIn->stateMachine, RX_STATE_WAIT_0x81);
				return;
			}
		}
	}
}


static void rxState_cb_wait0x81_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_rpc_protocolParser_t *const rppIn = (cxa_rpc_protocolParser_t *const)userVarIn;
	cxa_assert(rppIn);
	
	uint8_t rxByte;
	cxa_ioStream_readStatus_t readStat = cxa_ioStream_readByte(rppIn->ioStream, &rxByte);
	if( readStat == CXA_IOSTREAM_READSTAT_ERROR ) { handleIoException(rppIn); return; }
	else if( readStat == CXA_IOSTREAM_READSTAT_GOTDATA )
	{
		if( rxByte == 0x81 )
		{
			// we have a valid second header byte
			cxa_fixedByteBuffer_append_uint8(rppIn->currRxMsg->buffer, rxByte);
			cxa_stateMachine_transition(&rppIn->stateMachine, RX_STATE_WAIT_LEN);
			return;
		}
		else
		{
			// we have an invalid second header byte
			cxa_stateMachine_transition(&rppIn->stateMachine, RX_STATE_WAIT_0x80);
			return;
		}
	}
}


static void rxState_cb_waitLen_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_rpc_protocolParser_t *const rppIn = (cxa_rpc_protocolParser_t *const)userVarIn;
	cxa_assert(rppIn);
	
	uint8_t rxByte;
	cxa_ioStream_readStatus_t readStat = cxa_ioStream_readByte(rppIn->ioStream, &rxByte);
	if( readStat == CXA_IOSTREAM_READSTAT_ERROR ) { handleIoException(rppIn); return; }
	else if( readStat == CXA_IOSTREAM_READSTAT_GOTDATA )
	{
		// just append the byte
		cxa_fixedByteBuffer_append_uint8(rppIn->currRxMsg->buffer, rxByte);
		if( cxa_fixedByteBuffer_getSize_bytes(rppIn->currRxMsg->buffer) == 4 )
		{
			// we have all of our length bytes...make sure it's valid
			uint16_t len_bytes;
			if( cxa_fixedByteBuffer_get_uint16LE(rppIn->currRxMsg->buffer, 2, len_bytes) && (len_bytes >= 3) ) cxa_stateMachine_transition(&rppIn->stateMachine, RX_STATE_WAIT_DATA_BYTES);
			return;
		}
	}
}


static void rxState_cb_waitDataBytes_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_rpc_protocolParser_t *const rppIn = (cxa_rpc_protocolParser_t *const)userVarIn;
	cxa_assert(rppIn);
	
	// get our expected size
	uint16_t expectedSize_bytes;
	if( !cxa_fixedByteBuffer_get_uint16LE(rppIn->currRxMsg->buffer, 2, expectedSize_bytes) )
	{
		cxa_stateMachine_transition(&rppIn->stateMachine, RX_STATE_WAIT_0x80);
		return;
	}
	
	// do a limited number of iterations
	for( uint8_t i = 0; i < MAX_NUM_RX_BYTES_PER_UPDATE; i++ )
	{
		size_t currSize_bytes = cxa_fixedByteBuffer_getSize_bytes(rppIn->currRxMsg->buffer) - 4;
		
		if( currSize_bytes < expectedSize_bytes )
		{
			// we have more bytes to receive
			uint8_t rxByte;
			cxa_ioStream_readStatus_t readStat = cxa_ioStream_readByte(rppIn->ioStream, &rxByte);
			if( readStat == CXA_IOSTREAM_READSTAT_ERROR ) { handleIoException(rppIn); return; }
			else if( readStat == CXA_IOSTREAM_READSTAT_GOTDATA )
			{
				cxa_fixedByteBuffer_append_uint8(rppIn->currRxMsg->buffer, rxByte);
			}
		}
		else
		{
			// we're done receiving our data bytes
			cxa_stateMachine_transition(&rppIn->stateMachine, RX_STATE_PROCESS_MESSAGE);
			return;
		}
	}
}


static void rxState_cb_processMessage_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_rpc_protocolParser_t *const rppIn = (cxa_rpc_protocolParser_t *const)userVarIn;
	cxa_assert(rppIn);
	
	size_t currSize_bytes = cxa_fixedByteBuffer_getSize_bytes(rppIn->currRxMsg->buffer);
	
	uint8_t tmpVal8;
	uint16_t tmpVal16;

	// make sure our message is kosher
	if( (currSize_bytes >= 6) &&
		(cxa_fixedByteBuffer_get_uint8(rppIn->currRxMsg->buffer, 0, tmpVal8) && (tmpVal8 == 0x80)) &&
		(cxa_fixedByteBuffer_get_uint8(rppIn->currRxMsg->buffer, 1, tmpVal8) && (tmpVal8 == 0x81)) &&
		(cxa_fixedByteBuffer_get_uint16LE(rppIn->currRxMsg->buffer, 2, tmpVal16) && (tmpVal16 == (currSize_bytes-4))) &&
		(cxa_fixedByteBuffer_get_uint8(rppIn->currRxMsg->buffer, currSize_bytes-1, tmpVal8) && (tmpVal8 == 0x82)) )
	{
		// we have a valid message...check our version number
		uint8_t versionNum = 0xFF;
		if( !cxa_fixedByteBuffer_get_uint8(rppIn->currRxMsg->buffer, 4, versionNum) || (versionNum != ((PROTOCOL_VERSION<<4) | rppIn->userProtoVersion)) )
		{
			// invalid version number...
			cxa_logger_debug(&rppIn->logger, "message received for incorrect protocol version: 0x%02X", versionNum);
			
			// notify our protocol listeners
			cxa_array_iterate(&rppIn->protocolListeners, currEntry, cxa_rpc_protocolParser_protoListener_entry_t)
			{
				if( currEntry == NULL ) continue;
				
				if( currEntry->cb_invalidVer != NULL ) currEntry->cb_invalidVer(rppIn->currRxMsg->buffer, currEntry->userVar);
			}			
		}
		else
		{
			// we have a valid version number...get our data bytes
			cxa_logger_trace(&rppIn->logger, "message received...parsing");

			if( cxa_rpc_message_validateReceivedBytes(rppIn->currRxMsg, 5, currSize_bytes-1) )
			{
				cxa_logger_trace(&rppIn->logger, "message validated successfully");

				cxa_array_iterate(&rppIn->messageListeners, currEntry, cxa_rpc_protocolParser_messageListener_entry_t)
				{
					if( currEntry == NULL ) continue;

					if( currEntry->cb != NULL )
					{
						cxa_logger_trace(&rppIn->logger, "calling registered callback @ %p", currEntry->cb);
						currEntry->cb(rppIn->currRxMsg, currEntry->userVar);
					}
				}

				uint8_t refCount = cxa_rpc_messageFactory_getReferenceCountForMessage(rppIn->currRxMsg);
				// make sure there wasn't some weirdness happening
				if( refCount == 0 )
				{
					cxa_logger_warn(&rppIn->logger, "over-freed rx buffer (%p)", rppIn->currRxMsg);
					cxa_stateMachine_transition(&rppIn->stateMachine, RX_STATE_ERROR);
					cxa_stateMachine_update(&rppIn->stateMachine);
				}
				else if( refCount > 1 )
				{
					// we need to reserve another buffer (this one is still being used)

					// release _our_ lock on the message
					cxa_rpc_messageFactory_decrementMessageRefCount(rppIn->currRxMsg);

					// get a new message
					rppIn->currRxMsg = cxa_rpc_messageFactory_getFreeMessage_empty();
					if( rppIn->currRxMsg == NULL )
					{
						cxa_logger_warn(&rppIn->logger, "could not reserve free rx buffer");
						cxa_stateMachine_transition(&rppIn->stateMachine, RX_STATE_ERROR);
						cxa_stateMachine_update(&rppIn->stateMachine);
					}
				}
				else
				{
					// we need to reset our current buffer for re-use
					cxa_rpc_message_initEmpty(rppIn->currRxMsg, rppIn->currRxMsg->buffer);
				}
			}
			else cxa_logger_debug(&rppIn->logger, "invalid message received");
		}
	}
	else cxa_logger_debug(&rppIn->logger, "improperly formatted message received");
	
	// no matter what, we'll reset and wait for more data
	cxa_stateMachine_transition(&rppIn->stateMachine, RX_STATE_WAIT_0x80);
}


static void rxState_cb_error_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_rpc_protocolParser_t *const rppIn = (cxa_rpc_protocolParser_t *const)userVarIn;
	cxa_assert(rppIn);
	
	cxa_logger_error(&rppIn->logger, "underlying serial device is broken, protocol parser is inoperable");

	// notify our protocol listeners
	cxa_array_iterate(&rppIn->protocolListeners, currEntry, cxa_rpc_protocolParser_protoListener_entry_t)
	{
		if( currEntry == NULL ) continue;

		if( currEntry->cb_exception != NULL ) currEntry->cb_exception(currEntry->userVar);
	}
}
