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
#include "cxa_serial_protocolParser.h"


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
static uint8_t getBufferRefCount(cxa_serial_protocolParser_t *const sppIn, cxa_fixedByteBuffer_t *const dataBytesIn);
static void handleIoException(cxa_serial_protocolParser_t *const sppIn);
static void rxState_cb_idle_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_idle_leave(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_wait0x80_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_wait0x81_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_waitLen_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_waitDataBytes_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_processMessage_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void rxState_cb_error_enter(cxa_stateMachine_t *const smIn, void *userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_serial_protocolParser_init(cxa_serial_protocolParser_t *const sppIn, uint8_t userProtoVersionIn)
{
	cxa_assert(sppIn);
	cxa_assert(userProtoVersionIn <= 15);
	
	// save our references
	sppIn->userProtoVersion = userProtoVersionIn;
	
	// setup our logger
	cxa_logger_init(&sppIn->logger, "protocolParser");
	
	// setup our message pool
	for( size_t i = 0; i < (sizeof(sppIn->msgPool)/sizeof(*sppIn->msgPool)); i++ )
	{
		cxa_serial_protocolParser_msgBuffer_t *currBuffer = (cxa_serial_protocolParser_msgBuffer_t*)&sppIn->msgPool[i];
		currBuffer->refCount = 0;
		cxa_fixedByteBuffer_init(&currBuffer->buffer, (void*)currBuffer->buffer_raw, sizeof(currBuffer->buffer_raw));
		
		cxa_logger_trace(&sppIn->logger, "buffer %p added to pool", &currBuffer->buffer);
	}
	
	// setup our listeners
	cxa_array_init(&sppIn->protocolListeners, sizeof(*sppIn->protocolListeners_raw), (void*)sppIn->protocolListeners_raw, sizeof(sppIn->protocolListeners_raw));
	cxa_array_init(&sppIn->messageListeners, sizeof(*sppIn->messageListeners_raw), (void*)sppIn->messageListeners_raw, sizeof(sppIn->messageListeners_raw));
	
	// setup our state machine
	cxa_stateMachine_init(&sppIn->stateMachine, "protocolParser");
	cxa_stateMachine_addState(&sppIn->stateMachine, RX_STATE_IDLE, "idle", rxState_cb_idle_enter, NULL, rxState_cb_idle_leave, (void*)sppIn);
	cxa_stateMachine_addState(&sppIn->stateMachine, RX_STATE_WAIT_0x80, "wait_0x80", NULL, rxState_cb_wait0x80_state, NULL, (void*)sppIn);
	cxa_stateMachine_addState(&sppIn->stateMachine, RX_STATE_WAIT_0x81, "wait_0x81", NULL, rxState_cb_wait0x81_state, NULL, (void*)sppIn);
	cxa_stateMachine_addState(&sppIn->stateMachine, RX_STATE_WAIT_LEN, "wait_len", NULL, rxState_cb_waitLen_state, NULL, (void*)sppIn);
	cxa_stateMachine_addState(&sppIn->stateMachine, RX_STATE_WAIT_DATA_BYTES, "wait_dataBytes", NULL, rxState_cb_waitDataBytes_state, NULL, (void*)sppIn);
	cxa_stateMachine_addState(&sppIn->stateMachine, RX_STATE_PROCESS_MESSAGE, "processMsg", NULL, rxState_cb_processMessage_state, NULL, (void*)sppIn);
	cxa_stateMachine_addState(&sppIn->stateMachine, RX_STATE_ERROR, "error", rxState_cb_error_enter, NULL, NULL, (void*)sppIn);
	
	// set our initial state
	cxa_stateMachine_transition(&sppIn->stateMachine, RX_STATE_IDLE);
	cxa_stateMachine_update(&sppIn->stateMachine);
}


void cxa_serial_protocolParser_setSerialDevice(cxa_serial_protocolParser_t *const sppIn, FILE *const fdIn)
{
	cxa_assert(sppIn);
	
	sppIn->serialDev = fdIn;
	
	if( sppIn->serialDev == NULL ) cxa_stateMachine_transition(&sppIn->stateMachine, RX_STATE_IDLE);
	else
	{
		cxa_logger_debug(&sppIn->logger, "serial device set to fd[%p]", fdIn);
		cxa_stateMachine_transition(&sppIn->stateMachine, RX_STATE_WAIT_0x80);
	}	
}


cxa_fixedByteBuffer_t* cxa_serial_protocolParser_reserveFreeBuffer(cxa_serial_protocolParser_t *const sppIn)
{
	cxa_assert(sppIn);
	
	for( size_t i = 0; i < (sizeof(sppIn->msgPool)/sizeof(*sppIn->msgPool)); i++ )
	{
		cxa_serial_protocolParser_msgBuffer_t *currBuffer = (cxa_serial_protocolParser_msgBuffer_t*)&sppIn->msgPool[i];
		if( currBuffer->refCount == 0 )
		{
			currBuffer->refCount++;
			cxa_fixedByteBuffer_clear(&currBuffer->buffer);
			
			cxa_logger_trace(&sppIn->logger, "buffer %p newly reserved", &currBuffer->buffer);
			return &currBuffer->buffer;
		}
	}
	
	return NULL;	
}


bool cxa_serial_protocolParser_reserveExistingBuffer(cxa_serial_protocolParser_t *const sppIn, cxa_fixedByteBuffer_t *const dataBytesIn)
{
	cxa_assert(sppIn);
	cxa_assert(dataBytesIn);
	
	for( size_t i = 0; i < (sizeof(sppIn->msgPool)/sizeof(*sppIn->msgPool)); i++ )
	{
		cxa_serial_protocolParser_msgBuffer_t *currBuffer = (cxa_serial_protocolParser_msgBuffer_t*)&sppIn->msgPool[i];
		
		if( &currBuffer->buffer == dataBytesIn )
		{
			cxa_assert(currBuffer->refCount != 255);
			currBuffer->refCount++;
			cxa_logger_trace(&sppIn->logger, "buffer %p referenced", &currBuffer->buffer);
			return true;
		}
	}
	
	return false;
}


void cxa_serial_protocolParser_freeReservedBuffer(cxa_serial_protocolParser_t *const sppIn, cxa_fixedByteBuffer_t *const dataBytesIn)
{
	cxa_assert(sppIn);
	cxa_assert(dataBytesIn);
	
	for( size_t i = 0; i < (sizeof(sppIn->msgPool)/sizeof(*sppIn->msgPool)); i++ )
	{
		cxa_serial_protocolParser_msgBuffer_t *currBuffer = (cxa_serial_protocolParser_msgBuffer_t*)&sppIn->msgPool[i];
		
		if( &currBuffer->buffer == dataBytesIn )
		{
			cxa_assert(currBuffer->refCount != 0);
			currBuffer->refCount--;
			if( currBuffer->refCount == 0 ) cxa_logger_trace(&sppIn->logger, "buffer %p freed", &currBuffer->buffer);
			else cxa_logger_trace(&sppIn->logger, "buffer %p dereferenced", &currBuffer->buffer);
			return;
		}
	}	
}


bool cxa_serial_protocolParser_writeMessage(cxa_serial_protocolParser_t *const sppIn, cxa_fixedByteBuffer_t *const dataBytesIn)
{
	cxa_assert(sppIn);
	cxa_assert(cxa_fixedByteBuffer_getCurrSize(dataBytesIn) <= (65535-3));
	
	rxState_t currState = cxa_stateMachine_getCurrentState(&sppIn->stateMachine);
	if( (sppIn->serialDev == NULL) || (currState == RX_STATE_IDLE) || (currState == RX_STATE_ERROR) ) return false;
	
	if( fputc(0x80, sppIn->serialDev) == EOF ) { handleIoException(sppIn); return false; }
	if( fputc(0x81, sppIn->serialDev) == EOF ) { handleIoException(sppIn); return false; }
	
	size_t len = ((dataBytesIn != NULL) ? cxa_fixedByteBuffer_getCurrSize(dataBytesIn) : 0) + 2;
	if( fputc(((len & 0x00FF) >> 0), sppIn->serialDev) == EOF ) { handleIoException(sppIn); return false; }
	if( fputc(((len & 0xFF00) >> 8), sppIn->serialDev) == EOF ) { handleIoException(sppIn); return false; }
	
	if( fputc(((PROTOCOL_VERSION << 4) | sppIn->userProtoVersion), sppIn->serialDev) == EOF ) { handleIoException(sppIn); return false; }
	
	if( dataBytesIn != NULL )
	{
		if( !cxa_fixedByteBuffer_writeToFile_bytes(dataBytesIn, sppIn->serialDev) ) { handleIoException(sppIn); return false; }
	}	
	
	if( fputc(0x82, sppIn->serialDev) == EOF ) { handleIoException(sppIn); return false; }
	
	return true;
}


void cxa_serial_protocolParser_addProtocolListener(cxa_serial_protocolParser_t *const sppIn,
	cxa_serial_protocolParser_cb_invalidVersionNumber_t cb_invalidVerIn,
	cxa_serial_protocolParser_cb_ioExceptionOccurred_t cb_exceptionIn,
	void *const userVarIn)
{
	cxa_assert(sppIn);
	
	// create and add our new entry
	cxa_serial_protocolParser_protoListener_entry_t newEntry = {.cb_invalidVer=cb_invalidVerIn, .cb_exception=cb_exceptionIn, .userVar=userVarIn};
	cxa_assert( cxa_array_append(&sppIn->protocolListeners, &newEntry) );
}


void cxa_serial_protocolParser_addMessageListener(cxa_serial_protocolParser_t *const sppIn,
	cxa_serial_protocolParser_cb_messageReceived_t cb_msgRxIn,
	void *const userVarIn)
{
	cxa_assert(sppIn);
	
	// create and add our new entry
	cxa_serial_protocolParser_messageListener_entry_t newEntry = {.cb=cb_msgRxIn, .userVar=userVarIn};
	cxa_assert( cxa_array_append(&sppIn->messageListeners, &newEntry) );
}


void cxa_serial_protocolParser_update(cxa_serial_protocolParser_t *const sppIn)
{
	cxa_assert(sppIn);
	
	cxa_stateMachine_update(&sppIn->stateMachine);
}


// ******** local function implementations ********
static uint8_t getBufferRefCount(cxa_serial_protocolParser_t *const sppIn, cxa_fixedByteBuffer_t *const dataBytesIn)
{
	cxa_assert(sppIn);
	
	for( size_t i = 0; i < (sizeof(sppIn->msgPool)/sizeof(*sppIn->msgPool)); i++ )
	{
		cxa_serial_protocolParser_msgBuffer_t *currBuffer = (cxa_serial_protocolParser_msgBuffer_t*)&sppIn->msgPool[i];
		if( &currBuffer->buffer == dataBytesIn ) return currBuffer->refCount;
	}
	
	return 0;
}


static void handleIoException(cxa_serial_protocolParser_t *const sppIn)
{
	cxa_assert(sppIn);
	
	cxa_logger_error(&sppIn->logger, "exception occurred with underlying serial device");
	
	// notify our protocol listeners
	for( size_t i = 0; i < cxa_array_getSize_elems(&sppIn->protocolListeners); i++ )
	{
		cxa_serial_protocolParser_protoListener_entry_t *currEntry = (cxa_serial_protocolParser_protoListener_entry_t*)cxa_array_getAtIndex(&sppIn->protocolListeners, i);
		if( currEntry == NULL ) continue;
		
		if( currEntry->cb_exception != NULL ) currEntry->cb_exception(currEntry->userVar);
	}
}


static void rxState_cb_idle_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_serial_protocolParser_t *const sppIn = (cxa_serial_protocolParser_t *const)userVarIn;
	cxa_assert(sppIn);
	
	cxa_logger_info(&sppIn->logger, "becoming idle");
	
	// free all of the messages in our message pool
	for( size_t i = 0; i < (sizeof(sppIn->msgPool)/sizeof(*sppIn->msgPool)); i++ )
	{
		cxa_serial_protocolParser_msgBuffer_t *currBuffer = (cxa_serial_protocolParser_msgBuffer_t*)&sppIn->msgPool[i];
		currBuffer->refCount = 0;
	}
}


static void rxState_cb_idle_leave(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_serial_protocolParser_t *const sppIn = (cxa_serial_protocolParser_t *const)userVarIn;
	cxa_assert(sppIn);
	
	// try to reserve a free RX buffer
	sppIn->currRxBuffer = cxa_serial_protocolParser_reserveFreeBuffer(sppIn);
	if( sppIn->currRxBuffer == NULL )
	{
		cxa_logger_warn(&sppIn->logger, "could not reserve free rx buffer");
		cxa_stateMachine_transition(&sppIn->stateMachine, RX_STATE_ERROR);
		cxa_stateMachine_update(&sppIn->stateMachine);
	}
	else cxa_logger_info(&sppIn->logger, "becoming active");
}


static void rxState_cb_wait0x80_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_serial_protocolParser_t *const sppIn = (cxa_serial_protocolParser_t *const)userVarIn;
	cxa_assert(sppIn);
	
	for( uint8_t i = 0; i < MAX_NUM_RX_BYTES_PER_UPDATE; i++ )
	{
		int rxByte = fgetc(sppIn->serialDev);
		if( rxByte == EOF ) return;
		else if( rxByte == 0x80 )
		{
			// we've gotten our first header byte
			cxa_fixedByteBuffer_clear(sppIn->currRxBuffer);
			cxa_fixedByteBuffer_append_byte(sppIn->currRxBuffer, (uint8_t)rxByte);
			
			cxa_stateMachine_transition(&sppIn->stateMachine, RX_STATE_WAIT_0x81);
			return;
		}
	}
}


static void rxState_cb_wait0x81_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_serial_protocolParser_t *const sppIn = (cxa_serial_protocolParser_t *const)userVarIn;
	cxa_assert(sppIn);
	
	int rxByte = fgetc(sppIn->serialDev);
	if( rxByte == EOF ) return;
	else if( rxByte == 0x81 ) 
	{
		// we have a valid second header byte
		cxa_fixedByteBuffer_append_byte(sppIn->currRxBuffer, (uint8_t)rxByte);
		cxa_stateMachine_transition(&sppIn->stateMachine, RX_STATE_WAIT_LEN);
		return;
	}
	else
	{
		// we have an invalid second header byte
		cxa_stateMachine_transition(&sppIn->stateMachine, RX_STATE_WAIT_0x80);
		return;
	}
}


static void rxState_cb_waitLen_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_serial_protocolParser_t *const sppIn = (cxa_serial_protocolParser_t *const)userVarIn;
	cxa_assert(sppIn);
	
	int rxByte = fgetc(sppIn->serialDev);
	if( rxByte == EOF ) return;
	
	// just append the byte
	cxa_fixedByteBuffer_append_byte(sppIn->currRxBuffer, (uint8_t)rxByte);
	if( cxa_fixedByteBuffer_getCurrSize(sppIn->currRxBuffer) == 4 )
	{
		// we have all of our length bytes...make sure it's valid
		if( cxa_fixedByteBuffer_getUint16_LE(sppIn->currRxBuffer, 2) >= 3 ) cxa_stateMachine_transition(&sppIn->stateMachine, RX_STATE_WAIT_DATA_BYTES);
		cxa_stateMachine_transition(&sppIn->stateMachine, RX_STATE_WAIT_DATA_BYTES);
		return;
	}
}


static void rxState_cb_waitDataBytes_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_serial_protocolParser_t *const sppIn = (cxa_serial_protocolParser_t *const)userVarIn;
	cxa_assert(sppIn);
	
	// get our expected size
	uint16_t expectedSize_bytes = cxa_fixedByteBuffer_getUint16_LE(sppIn->currRxBuffer, 2);
	
	// do a limited number of iterations
	for( uint8_t i = 0; i < MAX_NUM_RX_BYTES_PER_UPDATE; i++ )
	{
		size_t currSize_bytes = cxa_fixedByteBuffer_getCurrSize(sppIn->currRxBuffer) - 4;
		
		if( currSize_bytes < expectedSize_bytes )
		{
			// we have more bytes to receive
			int rxByte = fgetc(sppIn->serialDev);
			if( rxByte == EOF ) return;
			
			cxa_fixedByteBuffer_append_byte(sppIn->currRxBuffer, (uint8_t)rxByte);
		}
		else
		{
			// we're done receiving our data bytes
			cxa_stateMachine_transition(&sppIn->stateMachine, RX_STATE_PROCESS_MESSAGE);
			return;
		}
	}
}


static void rxState_cb_processMessage_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_serial_protocolParser_t *const sppIn = (cxa_serial_protocolParser_t *const)userVarIn;
	cxa_assert(sppIn);
	
	size_t currSize_bytes = cxa_fixedByteBuffer_getCurrSize(sppIn->currRxBuffer);
	
	// make sure our message is kosher
	if( (currSize_bytes >= 6) &&
	(*cxa_fixedByteBuffer_getAtIndex(sppIn->currRxBuffer, 0) == 0x80) &&
	(*cxa_fixedByteBuffer_getAtIndex(sppIn->currRxBuffer, 1) == 0x81) &&
	(cxa_fixedByteBuffer_getUint16_LE(sppIn->currRxBuffer, 2) == (currSize_bytes-4)) &&
	(*cxa_fixedByteBuffer_getAtIndex(sppIn->currRxBuffer, currSize_bytes-1) == 0x82) )
	{
		// we have a valid message...check our version number
		uint8_t versionNum = *cxa_fixedByteBuffer_getAtIndex(sppIn->currRxBuffer, 4);
		if( versionNum != ((PROTOCOL_VERSION<<4) | sppIn->userProtoVersion) )
		{
			// invalid version number...
			cxa_logger_debug(&sppIn->logger, "message received for incorrect protocol version: 0x%02X", versionNum);
			
			// notify our protocol listeners
			for( size_t i = 0; i < cxa_array_getSize_elems(&sppIn->protocolListeners); i++ )
			{
				cxa_serial_protocolParser_protoListener_entry_t *currEntry = (cxa_serial_protocolParser_protoListener_entry_t*)cxa_array_getAtIndex(&sppIn->protocolListeners, i);
				if( currEntry == NULL ) continue;
				
				if( currEntry->cb_invalidVer != NULL ) currEntry->cb_invalidVer(sppIn->currRxBuffer, currEntry->userVar);
			}			
		}
		else
		{
			// we have a valid version number...get our data bytes
			cxa_logger_trace(&sppIn->logger, "message received");
			cxa_fixedByteBuffer_t dataBytes;
			cxa_fixedByteBuffer_init_subsetOfData(&dataBytes, sppIn->currRxBuffer, 5, (currSize_bytes - 6));
			for( size_t i = 0; i < cxa_array_getSize_elems(&sppIn->messageListeners); i++ )
			{
				cxa_serial_protocolParser_messageListener_entry_t *currEntry = (cxa_serial_protocolParser_messageListener_entry_t*)cxa_array_getAtIndex(&sppIn->messageListeners, i);
				if( currEntry == NULL ) continue;
			
				if( currEntry->cb != NULL )
				{
					cxa_logger_trace(&sppIn->logger, "calling registered callback @ %p", currEntry->cb);
					currEntry->cb(&dataBytes, currEntry->userVar);
				}				
			}
			
			uint8_t refCount = getBufferRefCount(sppIn, sppIn->currRxBuffer);
			// make sure there wasn't some weirdness happening
			if( refCount == 0 )
			{
				cxa_logger_warn(&sppIn->logger, "over-freed rx buffer");
				cxa_stateMachine_transition(&sppIn->stateMachine, RX_STATE_ERROR);
				cxa_stateMachine_update(&sppIn->stateMachine);
			}
			else if( refCount > 1 )
			{
				// we need to reserve another buffer (this one is still being used)
				sppIn->currRxBuffer = cxa_serial_protocolParser_reserveFreeBuffer(sppIn);
				if( sppIn->currRxBuffer == NULL )
				{
					cxa_logger_warn(&sppIn->logger, "could not reserve free rx buffer");
					cxa_stateMachine_transition(&sppIn->stateMachine, RX_STATE_ERROR);
					cxa_stateMachine_update(&sppIn->stateMachine);
				}				
			}
		}
	}else cxa_logger_debug(&sppIn->logger, "improperly formatted message received");
	
	// no matter what, we'll reset and wait for more data
	cxa_stateMachine_transition(&sppIn->stateMachine, RX_STATE_WAIT_0x80);
}


static void rxState_cb_error_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_serial_protocolParser_t *const sppIn = (cxa_serial_protocolParser_t *const)userVarIn;
	cxa_assert(sppIn);
	
	cxa_logger_error(&sppIn->logger, "underlying serial device is broken, protocol parser is inoperable");
}