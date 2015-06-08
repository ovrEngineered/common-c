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
static uint8_t getBufferRefCount(cxa_rpc_protocolParser_t *const rppIn, cxa_fixedByteBuffer_t *const dataBytesIn);
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

	// setup our logger
	cxa_logger_init(&rppIn->logger, "protocolParser");

	// setup our message pool
	for( size_t i = 0; i < (sizeof(rppIn->msgPool)/sizeof(*rppIn->msgPool)); i++ )
	{
		cxa_rpc_protocolParser_msgBuffer_t *currBuffer = (cxa_rpc_protocolParser_msgBuffer_t*)&rppIn->msgPool[i];
		currBuffer->refCount = 0;
		cxa_fixedByteBuffer_init(&currBuffer->buffer, (void*)currBuffer->buffer_raw, sizeof(currBuffer->buffer_raw));
		
		cxa_logger_trace(&rppIn->logger, "buffer %p added to pool", &currBuffer->buffer);
	}

	// setup our listeners
	cxa_array_init(&rppIn->protocolListeners, sizeof(*rppIn->protocolListeners_raw), (void*)rppIn->protocolListeners_raw, sizeof(rppIn->protocolListeners_raw));
	cxa_array_init(&rppIn->messageListeners, sizeof(*rppIn->messageListeners_raw), (void*)rppIn->messageListeners_raw, sizeof(rppIn->messageListeners_raw));

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


cxa_fixedByteBuffer_t* cxa_rpc_protocolParser_reserveFreeBuffer(cxa_rpc_protocolParser_t *const rppIn)
{
	cxa_assert(rppIn);
	
	for( size_t i = 0; i < (sizeof(rppIn->msgPool)/sizeof(*rppIn->msgPool)); i++ )
	{
		cxa_rpc_protocolParser_msgBuffer_t *currBuffer = (cxa_rpc_protocolParser_msgBuffer_t*)&rppIn->msgPool[i];
		if( currBuffer->refCount == 0 )
		{
			currBuffer->refCount++;
			cxa_fixedByteBuffer_clear(&currBuffer->buffer);
			
			cxa_logger_trace(&rppIn->logger, "buffer %p newly reserved", &currBuffer->buffer);
			return &currBuffer->buffer;
		}
	}
	
	return NULL;	
}


bool cxa_rpc_protocolParser_reserveExistingBuffer(cxa_rpc_protocolParser_t *const rppIn, cxa_fixedByteBuffer_t *const dataBytesIn)
{
	cxa_assert(rppIn);
	cxa_assert(dataBytesIn);
	
	for( size_t i = 0; i < (sizeof(rppIn->msgPool)/sizeof(*rppIn->msgPool)); i++ )
	{
		cxa_rpc_protocolParser_msgBuffer_t *currBuffer = (cxa_rpc_protocolParser_msgBuffer_t*)&rppIn->msgPool[i];
		
		if( &currBuffer->buffer == dataBytesIn )
		{
			cxa_assert(currBuffer->refCount != 255);
			currBuffer->refCount++;
			cxa_logger_trace(&rppIn->logger, "buffer %p referenced", &currBuffer->buffer);
			return true;
		}
	}
	
	return false;
}


void cxa_rpc_protocolParser_freeReservedBuffer(cxa_rpc_protocolParser_t *const rppIn, cxa_fixedByteBuffer_t *const dataBytesIn)
{
	cxa_assert(rppIn);
	cxa_assert(dataBytesIn);
	
	for( size_t i = 0; i < (sizeof(rppIn->msgPool)/sizeof(*rppIn->msgPool)); i++ )
	{
		cxa_rpc_protocolParser_msgBuffer_t *currBuffer = (cxa_rpc_protocolParser_msgBuffer_t*)&rppIn->msgPool[i];
		
		if( &currBuffer->buffer == dataBytesIn )
		{
			cxa_assert(currBuffer->refCount != 0);
			currBuffer->refCount--;
			if( currBuffer->refCount == 0 ) cxa_logger_trace(&rppIn->logger, "buffer %p freed", &currBuffer->buffer);
			else cxa_logger_trace(&rppIn->logger, "buffer %p dereferenced", &currBuffer->buffer);
			return;
		}
	}	
}


bool cxa_rpc_protocolParser_writeMessage(cxa_rpc_protocolParser_t *const rppIn, cxa_fixedByteBuffer_t *const dataBytesIn)
{
	cxa_assert(rppIn);
	cxa_assert(cxa_fixedByteBuffer_getCurrSize(dataBytesIn) <= (65535-3));

	rxState_t currState = cxa_stateMachine_getCurrentState(&rppIn->stateMachine);
	if( (currState == RX_STATE_ERROR) || !cxa_ioStream_isBound(rppIn->ioStream) ) return false;

	if( !cxa_ioStream_writeByte(rppIn->ioStream, 0x80) ) { handleIoException(rppIn); return false; }
	if( !cxa_ioStream_writeByte(rppIn->ioStream, 0x81) ) { handleIoException(rppIn); return false; }

	size_t len = ((dataBytesIn != NULL) ? cxa_fixedByteBuffer_getCurrSize(dataBytesIn) : 0) + 2;
	if( !cxa_ioStream_writeByte(rppIn->ioStream, ((len & 0x00FF) >> 0)) ) { handleIoException(rppIn); return false; }
	if( !cxa_ioStream_writeByte(rppIn->ioStream, ((len & 0xFF00) >> 8)) ) { handleIoException(rppIn); return false; }
	if( !cxa_ioStream_writeByte(rppIn->ioStream, ((PROTOCOL_VERSION << 4) | rppIn->userProtoVersion)) ) { handleIoException(rppIn); return false; }

	if( dataBytesIn != NULL )
	{
		if( !cxa_ioStream_writeFixedByteBuffer(rppIn->ioStream, dataBytesIn) ) { handleIoException(rppIn); return false; }
	}
	
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
static uint8_t getBufferRefCount(cxa_rpc_protocolParser_t *const rppIn, cxa_fixedByteBuffer_t *const dataBytesIn)
{
	cxa_assert(rppIn);
	
	for( size_t i = 0; i < (sizeof(rppIn->msgPool)/sizeof(*rppIn->msgPool)); i++ )
	{
		cxa_rpc_protocolParser_msgBuffer_t *currBuffer = (cxa_rpc_protocolParser_msgBuffer_t*)&rppIn->msgPool[i];
		if( &currBuffer->buffer == dataBytesIn ) return currBuffer->refCount;
	}
	
	return 0;
}


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
	
	// free all of the messages in our message pool
	for( size_t i = 0; i < (sizeof(rppIn->msgPool)/sizeof(*rppIn->msgPool)); i++ )
	{
		cxa_rpc_protocolParser_msgBuffer_t *currBuffer = (cxa_rpc_protocolParser_msgBuffer_t*)&rppIn->msgPool[i];
		currBuffer->refCount = 0;
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
	rppIn->currRxBuffer = cxa_rpc_protocolParser_reserveFreeBuffer(rppIn);
	if( rppIn->currRxBuffer == NULL )
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
				cxa_fixedByteBuffer_clear(rppIn->currRxBuffer);
				cxa_fixedByteBuffer_append_uint8(rppIn->currRxBuffer, rxByte);

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
			cxa_fixedByteBuffer_append_uint8(rppIn->currRxBuffer, rxByte);
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
		cxa_fixedByteBuffer_append_uint8(rppIn->currRxBuffer, rxByte);
		if( cxa_fixedByteBuffer_getCurrSize(rppIn->currRxBuffer) == 4 )
		{
			// we have all of our length bytes...make sure it's valid
			if( cxa_fixedByteBuffer_get_uint16LE(rppIn->currRxBuffer, 2) >= 3 ) cxa_stateMachine_transition(&rppIn->stateMachine, RX_STATE_WAIT_DATA_BYTES);
			return;
		}
	}
}


static void rxState_cb_waitDataBytes_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_rpc_protocolParser_t *const rppIn = (cxa_rpc_protocolParser_t *const)userVarIn;
	cxa_assert(rppIn);
	
	// get our expected size
	uint16_t expectedSize_bytes = cxa_fixedByteBuffer_get_uint16LE(rppIn->currRxBuffer, 2);
	
	// do a limited number of iterations
	for( uint8_t i = 0; i < MAX_NUM_RX_BYTES_PER_UPDATE; i++ )
	{
		size_t currSize_bytes = cxa_fixedByteBuffer_getCurrSize(rppIn->currRxBuffer) - 4;
		
		if( currSize_bytes < expectedSize_bytes )
		{
			// we have more bytes to receive
			uint8_t rxByte;
			cxa_ioStream_readStatus_t readStat = cxa_ioStream_readByte(rppIn->ioStream, &rxByte);
			if( readStat == CXA_IOSTREAM_READSTAT_ERROR ) { handleIoException(rppIn); return; }
			else if( readStat == CXA_IOSTREAM_READSTAT_GOTDATA )
			{
				cxa_fixedByteBuffer_append_uint8(rppIn->currRxBuffer, rxByte);
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
	
	size_t currSize_bytes = cxa_fixedByteBuffer_getCurrSize(rppIn->currRxBuffer);
	
	// make sure our message is kosher
	if( (currSize_bytes >= 6) &&
	(cxa_fixedByteBuffer_get_uint8(rppIn->currRxBuffer, 0) == 0x80) &&
	(cxa_fixedByteBuffer_get_uint8(rppIn->currRxBuffer, 1) == 0x81) &&
	(cxa_fixedByteBuffer_get_uint16LE(rppIn->currRxBuffer, 2) == (currSize_bytes-4)) &&
	(cxa_fixedByteBuffer_get_uint8(rppIn->currRxBuffer, currSize_bytes-1) == 0x82) )
	{
		// we have a valid message...check our version number
		uint8_t versionNum = cxa_fixedByteBuffer_get_uint8(rppIn->currRxBuffer, 4);
		if( versionNum != ((PROTOCOL_VERSION<<4) | rppIn->userProtoVersion) )
		{
			// invalid version number...
			cxa_logger_debug(&rppIn->logger, "message received for incorrect protocol version: 0x%02X", versionNum);
			
			// notify our protocol listeners
			for( size_t i = 0; i < cxa_array_getSize_elems(&rppIn->protocolListeners); i++ )
			{
				cxa_rpc_protocolParser_protoListener_entry_t *currEntry = (cxa_rpc_protocolParser_protoListener_entry_t*)cxa_array_getAtIndex(&rppIn->protocolListeners, i);
				if( currEntry == NULL ) continue;
				
				if( currEntry->cb_invalidVer != NULL ) currEntry->cb_invalidVer(rppIn->currRxBuffer, currEntry->userVar);
			}			
		}
		else
		{
			// we have a valid version number...get our data bytes
			cxa_logger_trace(&rppIn->logger, "message received");
			cxa_fixedByteBuffer_t dataBytes;
			cxa_fixedByteBuffer_init_subsetOfData(&dataBytes, rppIn->currRxBuffer, 5, (currSize_bytes - 6));
			for( size_t i = 0; i < cxa_array_getSize_elems(&rppIn->messageListeners); i++ )
			{
				cxa_rpc_protocolParser_messageListener_entry_t *currEntry = (cxa_rpc_protocolParser_messageListener_entry_t*)cxa_array_getAtIndex(&rppIn->messageListeners, i);
				if( currEntry == NULL ) continue;
			
				if( currEntry->cb != NULL )
				{
					cxa_logger_trace(&rppIn->logger, "calling registered callback @ %p", currEntry->cb);
					currEntry->cb(&dataBytes, currEntry->userVar);
				}				
			}
			
			uint8_t refCount = getBufferRefCount(rppIn, rppIn->currRxBuffer);
			// make sure there wasn't some weirdness happening
			if( refCount == 0 )
			{
				cxa_logger_warn(&rppIn->logger, "over-freed rx buffer");
				cxa_stateMachine_transition(&rppIn->stateMachine, RX_STATE_ERROR);
				cxa_stateMachine_update(&rppIn->stateMachine);
			}
			else if( refCount > 1 )
			{
				// we need to reserve another buffer (this one is still being used)
				rppIn->currRxBuffer = cxa_rpc_protocolParser_reserveFreeBuffer(rppIn);
				if( rppIn->currRxBuffer == NULL )
				{
					cxa_logger_warn(&rppIn->logger, "could not reserve free rx buffer");
					cxa_stateMachine_transition(&rppIn->stateMachine, RX_STATE_ERROR);
					cxa_stateMachine_update(&rppIn->stateMachine);
				}				
			}
		}
	}else cxa_logger_debug(&rppIn->logger, "improperly formatted message received");
	
	// no matter what, we'll reset and wait for more data
	cxa_stateMachine_transition(&rppIn->stateMachine, RX_STATE_WAIT_0x80);
}


static void rxState_cb_error_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_rpc_protocolParser_t *const rppIn = (cxa_rpc_protocolParser_t *const)userVarIn;
	cxa_assert(rppIn);
	
	cxa_logger_error(&rppIn->logger, "underlying serial device is broken, protocol parser is inoperable");

	// notify our protocol listeners
	for( size_t i = 0; i < cxa_array_getSize_elems(&rppIn->protocolListeners); i++ )
	{
		cxa_rpc_protocolParser_protoListener_entry_t *currEntry = (cxa_rpc_protocolParser_protoListener_entry_t*)cxa_array_getAtIndex(&rppIn->protocolListeners, i);
		if( currEntry == NULL ) continue;

		if( currEntry->cb_exception != NULL ) currEntry->cb_exception(currEntry->userVar);
	}
}
