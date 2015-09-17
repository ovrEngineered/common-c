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
 */
#include "cxa_protocolParser.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdio.h>
#include <cxa_assert.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_DEBUG
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define MAX_NUM_RX_BYTES_PER_UPDATE		16


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
static void handleIoException(cxa_protocolParser_t *const ppIn);
static void rxState_cb_idle_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_idle_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_idle_leave(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_wait0x80_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_wait0x81_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_waitLen_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_waitDataBytes_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_processPacket_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_error_enter(cxa_stateMachine_t *const smIn, void *userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_protocolParser_init(cxa_protocolParser_t *const ppIn, cxa_ioStream_t* ioStreamIn, cxa_fixedByteBuffer_t* buffIn)
{
	cxa_assert(ppIn);
	cxa_assert(ioStreamIn);
	
	// save our references
	ppIn->ioStream = ioStreamIn;
	ppIn->currBuffer = buffIn;

	// setup our logger
	cxa_logger_init(&ppIn->logger, "protocolParser");

	// setup our listeners
	cxa_array_initStd(&ppIn->exceptionListeners, ppIn->exceptionListeners_raw);
	cxa_array_initStd(&ppIn->packetListeners, ppIn->packetListeners_raw);

	// setup our state machine
	cxa_stateMachine_init(&ppIn->stateMachine, "protocolParser");
	cxa_stateMachine_addState(&ppIn->stateMachine, RX_STATE_IDLE, "idle", rxState_cb_idle_enter, rxState_cb_idle_state, rxState_cb_idle_leave, (void*)ppIn);
	cxa_stateMachine_addState(&ppIn->stateMachine, RX_STATE_WAIT_0x80, "wait_0x80", NULL, rxState_cb_wait0x80_state, NULL, (void*)ppIn);
	cxa_stateMachine_addState(&ppIn->stateMachine, RX_STATE_WAIT_0x81, "wait_0x81", NULL, rxState_cb_wait0x81_state, NULL, (void*)ppIn);
	cxa_stateMachine_addState(&ppIn->stateMachine, RX_STATE_WAIT_LEN, "wait_len", NULL, rxState_cb_waitLen_state, NULL, (void*)ppIn);
	cxa_stateMachine_addState(&ppIn->stateMachine, RX_STATE_WAIT_DATA_BYTES, "wait_dataBytes", NULL, rxState_cb_waitDataBytes_state, NULL, (void*)ppIn);
	cxa_stateMachine_addState(&ppIn->stateMachine, RX_STATE_PROCESS_PACKET, "processPacket", NULL, rxState_cb_processPacket_state, NULL, (void*)ppIn);
	cxa_stateMachine_addState(&ppIn->stateMachine, RX_STATE_ERROR, "error", rxState_cb_error_enter, NULL, NULL, (void*)ppIn);

	// set our initial state
	cxa_stateMachine_transition(&ppIn->stateMachine, RX_STATE_IDLE);
	cxa_stateMachine_update(&ppIn->stateMachine);
}


void cxa_protocolParser_addExceptionListener(cxa_protocolParser_t *const ppIn,
		cxa_protocolParser_cb_ioExceptionOccurred_t cb_exceptionIn,
		void *const userVarIn)
{
	cxa_assert(ppIn);

	// create and add our new entry
	cxa_protocolParser_exceptionListener_entry_t newEntry = {.cb=cb_exceptionIn, .userVar=userVarIn};
	cxa_assert( cxa_array_append(&ppIn->exceptionListeners, &newEntry) );
}


void cxa_protocolParser_addPacketListener(cxa_protocolParser_t *const ppIn,
	cxa_protocolParser_cb_packetReceived_t cb_msgRxIn,
	void *const userVarIn)
{
	cxa_assert(ppIn);

	// create and add our new entry
	cxa_protocolParser_packetListener_entry_t newEntry = {.cb=cb_msgRxIn, .userVar=userVarIn};
	cxa_assert( cxa_array_append(&ppIn->packetListeners, &newEntry) );
}


void cxa_protocolParser_setBuffer(cxa_protocolParser_t *const ppIn, cxa_fixedByteBuffer_t* buffIn)
{
	cxa_assert(ppIn);

	rxState_t currState = cxa_stateMachine_getCurrentState(&ppIn->stateMachine);

	// handle our special cases
	if( buffIn == NULL)
	{
		ppIn->currBuffer = NULL;
		cxa_stateMachine_transition(&ppIn->stateMachine, RX_STATE_IDLE);
	}
	else if( currState == RX_STATE_PROCESS_PACKET )
	{
		ppIn->currBuffer = buffIn;
	}
	else if( currState == RX_STATE_IDLE )
	{
		// set the buffer
		// (state machine will take care of starting automatically)
		ppIn->currBuffer = buffIn;
	}
	else
	{
		// get to the idle state
		cxa_stateMachine_transition(&ppIn->stateMachine, RX_STATE_IDLE);
		cxa_protocolParser_update(ppIn);
		cxa_assert( cxa_stateMachine_getCurrentState(&ppIn->stateMachine) == RX_STATE_IDLE );

		// set the buffer
		// (state machine will take care of starting automatically)
		ppIn->currBuffer = buffIn;
	}
}


bool cxa_protocolParser_writePacket(cxa_protocolParser_t *const ppIn, cxa_fixedByteBuffer_t *const dataIn)
{
	cxa_assert(ppIn);

	// message _should_ be configured properly...get our size
	size_t msgSize_bytes = (dataIn != NULL) ? cxa_fixedByteBuffer_getSize_bytes(dataIn) : 0;
	cxa_assert(msgSize_bytes <= (65535-3));

	// make sure we're in a good state
	rxState_t currState = cxa_stateMachine_getCurrentState(&ppIn->stateMachine);
	if( (currState == RX_STATE_ERROR) || !cxa_ioStream_isBound(ppIn->ioStream) ) return false;

	// write our header
	if( !cxa_ioStream_writeByte(ppIn->ioStream, 0x80) ) { handleIoException(ppIn); return false; }
	if( !cxa_ioStream_writeByte(ppIn->ioStream, 0x81) ) { handleIoException(ppIn); return false; }

	size_t len = msgSize_bytes + 1;
	if( !cxa_ioStream_writeByte(ppIn->ioStream, ((len & 0x00FF) >> 0)) ) { handleIoException(ppIn); return false; }
	if( !cxa_ioStream_writeByte(ppIn->ioStream, ((len & 0xFF00) >> 8)) ) { handleIoException(ppIn); return false; }

	// write our data
	if( (dataIn != NULL) && !cxa_ioStream_writeFixedByteBuffer(ppIn->ioStream, dataIn) ) { handleIoException(ppIn); return false; }
	
	// write our footer
	if( !cxa_ioStream_writeByte(ppIn->ioStream, 0x82) ) { handleIoException(ppIn); return false; }
	return true;
}


void cxa_protocolParser_update(cxa_protocolParser_t *const ppIn)
{
	cxa_assert(ppIn);
	
	cxa_stateMachine_update(&ppIn->stateMachine);
}


// ******** local function implementations ********
static void handleIoException(cxa_protocolParser_t *const ppIn)
{
	cxa_assert(ppIn);
	
	cxa_logger_error(&ppIn->logger, "exception occurred with underlying serial device");
	
	cxa_stateMachine_transition(&ppIn->stateMachine, RX_STATE_ERROR);
}


static void rxState_cb_idle_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_t *const ppIn = (cxa_protocolParser_t *const)userVarIn;
	cxa_assert(ppIn);
	
	cxa_logger_info(&ppIn->logger, "becoming idle");
}


static void rxState_cb_idle_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_t *const ppIn = (cxa_protocolParser_t *const)userVarIn;
	cxa_assert(ppIn);

	// if we have a bound ioStream and a buffer, become active
	if( cxa_ioStream_isBound(ppIn->ioStream) && (ppIn->currBuffer != NULL) )
	{
		cxa_stateMachine_transition(&ppIn->stateMachine, RX_STATE_WAIT_0x80);
		return;
	}
}


static void rxState_cb_idle_leave(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_t *const ppIn = (cxa_protocolParser_t *const)userVarIn;
	cxa_assert(ppIn);
	
	cxa_logger_info(&ppIn->logger, "becoming active");
}


static void rxState_cb_wait0x80_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_t *const ppIn = (cxa_protocolParser_t *const)userVarIn;
	cxa_assert(ppIn);

	uint8_t rxByte;
	for( uint8_t i = 0; i < MAX_NUM_RX_BYTES_PER_UPDATE; i++ )
	{
		cxa_ioStream_readStatus_t readStat = cxa_ioStream_readByte(ppIn->ioStream, &rxByte);
		if( readStat == CXA_IOSTREAM_READSTAT_ERROR ) { handleIoException(ppIn); return; }
		else if( readStat == CXA_IOSTREAM_READSTAT_GOTDATA )
		{
			if( rxByte == 0x80 )
			{
				// we've gotten our first header byte
				cxa_fixedByteBuffer_clear(ppIn->currBuffer);
				cxa_fixedByteBuffer_append_uint8(ppIn->currBuffer, rxByte);

				cxa_stateMachine_transition(&ppIn->stateMachine, RX_STATE_WAIT_0x81);
				return;
			}
		}
	}
}


static void rxState_cb_wait0x81_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_t *const ppIn = (cxa_protocolParser_t *const)userVarIn;
	cxa_assert(ppIn);
	
	uint8_t rxByte;
	cxa_ioStream_readStatus_t readStat = cxa_ioStream_readByte(ppIn->ioStream, &rxByte);
	if( readStat == CXA_IOSTREAM_READSTAT_ERROR ) { handleIoException(ppIn); return; }
	else if( readStat == CXA_IOSTREAM_READSTAT_GOTDATA )
	{
		if( rxByte == 0x81 )
		{
			// we have a valid second header byte
			cxa_fixedByteBuffer_append_uint8(ppIn->currBuffer, rxByte);
			cxa_stateMachine_transition(&ppIn->stateMachine, RX_STATE_WAIT_LEN);
			return;
		}
		else
		{
			// we have an invalid second header byte
			cxa_stateMachine_transition(&ppIn->stateMachine, RX_STATE_WAIT_0x80);
			return;
		}
	}
}


static void rxState_cb_waitLen_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_t *const ppIn = (cxa_protocolParser_t *const)userVarIn;
	cxa_assert(ppIn);
	
	uint8_t rxByte;
	cxa_ioStream_readStatus_t readStat = cxa_ioStream_readByte(ppIn->ioStream, &rxByte);
	if( readStat == CXA_IOSTREAM_READSTAT_ERROR ) { handleIoException(ppIn); return; }
	else if( readStat == CXA_IOSTREAM_READSTAT_GOTDATA )
	{
		// just append the byte
		cxa_fixedByteBuffer_append_uint8(ppIn->currBuffer, rxByte);
		if( cxa_fixedByteBuffer_getSize_bytes(ppIn->currBuffer) == 4 )
		{
			// we have all of our length bytes...make sure it's valid
			uint16_t len_bytes;
			if( cxa_fixedByteBuffer_get_uint16LE(ppIn->currBuffer, 2, len_bytes) && (len_bytes >= 1) ) cxa_stateMachine_transition(&ppIn->stateMachine, RX_STATE_WAIT_DATA_BYTES);
			return;
		}
	}
}


static void rxState_cb_waitDataBytes_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_t *const ppIn = (cxa_protocolParser_t *const)userVarIn;
	cxa_assert(ppIn);
	
	// get our expected size
	uint16_t expectedSize_bytes;
	if( !cxa_fixedByteBuffer_get_uint16LE(ppIn->currBuffer, 2, expectedSize_bytes) )
	{
		cxa_stateMachine_transition(&ppIn->stateMachine, RX_STATE_WAIT_0x80);
		return;
	}
	
	// do a limited number of iterations
	for( uint8_t i = 0; i < MAX_NUM_RX_BYTES_PER_UPDATE; i++ )
	{
		size_t currSize_bytes = cxa_fixedByteBuffer_getSize_bytes(ppIn->currBuffer) - 4;
		
		if( currSize_bytes < expectedSize_bytes )
		{
			// we have more bytes to receive
			uint8_t rxByte;
			cxa_ioStream_readStatus_t readStat = cxa_ioStream_readByte(ppIn->ioStream, &rxByte);
			if( readStat == CXA_IOSTREAM_READSTAT_ERROR ) { handleIoException(ppIn); return; }
			else if( readStat == CXA_IOSTREAM_READSTAT_GOTDATA )
			{
				cxa_fixedByteBuffer_append_uint8(ppIn->currBuffer, rxByte);
			}
		}
		else
		{
			// we're done receiving our data bytes
			cxa_stateMachine_transition(&ppIn->stateMachine, RX_STATE_PROCESS_PACKET);
			return;
		}
	}
}


static void rxState_cb_processPacket_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_t *const ppIn = (cxa_protocolParser_t *const)userVarIn;
	cxa_assert(ppIn);
	
	size_t currSize_bytes = cxa_fixedByteBuffer_getSize_bytes(ppIn->currBuffer);
	
	uint8_t tmpVal8;
	uint16_t tmpVal16;

	// make sure our packet is kosher
	if( (currSize_bytes >= 5) &&
		(cxa_fixedByteBuffer_get_uint8(ppIn->currBuffer, 0, tmpVal8) && (tmpVal8 == 0x80)) &&
		(cxa_fixedByteBuffer_get_uint8(ppIn->currBuffer, 1, tmpVal8) && (tmpVal8 == 0x81)) &&
		(cxa_fixedByteBuffer_get_uint16LE(ppIn->currBuffer, 2, tmpVal16) && (tmpVal16 == (currSize_bytes-4))) &&
		(cxa_fixedByteBuffer_get_uint8(ppIn->currBuffer, currSize_bytes-1, tmpVal8) && (tmpVal8 == 0x82)) )
	{
		// we received a message
		cxa_logger_trace(&ppIn->logger, "message received...calling listeners");

		cxa_array_iterate(&ppIn->packetListeners, currEntry, cxa_protocolParser_packetListener_entry_t)
		{
			if( currEntry == NULL ) continue;

			cxa_fixedByteBuffer_t fbb_data;
			cxa_fixedByteBuffer_init_subBufferFixedSize(&fbb_data, ppIn->currBuffer, 4, (currSize_bytes-5));
			if( currEntry->cb != NULL )
			{
				currEntry->cb(&fbb_data, currEntry->userVar);
			}
		}
	}
	else cxa_logger_debug(&ppIn->logger, "improperly formatted message received");
	
	// no matter what, we'll reset and wait for more data
	cxa_stateMachine_transition(&ppIn->stateMachine, RX_STATE_WAIT_0x80);
}


static void rxState_cb_error_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_protocolParser_t *const ppIn = (cxa_protocolParser_t *const)userVarIn;
	cxa_assert(ppIn);
	
	cxa_logger_error(&ppIn->logger, "underlying serial device is broken, protocol parser is inoperable");

	// notify our exception listeners
	cxa_array_iterate(&ppIn->exceptionListeners, currEntry, cxa_protocolParser_exceptionListener_entry_t)
	{
		if( currEntry == NULL ) continue;

		if( currEntry->cb != NULL ) currEntry->cb(currEntry->userVar);
	}
}
