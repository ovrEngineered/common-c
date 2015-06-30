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
#include "cxa_rpc_messageFactory.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdint.h>
#include <cxa_assert.h>
#include <cxa_config.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_DEBUG
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#ifndef CXA_RPC_MSGFACTORY_BUFFER_SIZE_BYTES
	#define CXA_RPC_MSGFACTORY_BUFFER_SIZE_BYTES			256
#endif


// ******** local type definitions ********
typedef struct
{
	uint8_t refCount;

	cxa_rpc_message_t msg;
	cxa_fixedByteBuffer_t msgFbb;
	uint8_t msg_raw[CXA_RPC_MSGFACTORY_BUFFER_SIZE_BYTES];
}cxa_rpc_messageFactory_msgEntry_t;


// ******** local function prototypes ********
static cxa_rpc_messageFactory_msgEntry_t* getMsgEntryFromMessage(cxa_rpc_message_t *const msgIn);


// ********  local variable declarations *********
static bool isInit = false;
static cxa_logger_t logger;
static cxa_rpc_messageFactory_msgEntry_t msgPool[CXA_RPC_MSGFACTORY_POOL_NUM_MSGS];


// ******** global function implementations ********
void cxa_rpc_messageFactory_init(void)
{
	if( isInit ) return;

	// setup our logger
	cxa_logger_init(&logger, "rpcMsgFactory");

	// setup our message pool
	for( size_t i = 0; i < (sizeof(msgPool)/sizeof(*msgPool)); i++ )
	{
		cxa_rpc_messageFactory_msgEntry_t* currEntry = (cxa_rpc_messageFactory_msgEntry_t*)&msgPool[i];

		currEntry->refCount = 0;
		cxa_fixedByteBuffer_initStd(&currEntry->msgFbb, currEntry->msg_raw);

		cxa_logger_trace(&logger, "message %p added to pool", &currBuffer->msg);
	}

	isInit = true;
}


size_t cxa_rpc_messageFactory_getNumFreeMessages(void)
{
	if( !isInit ) cxa_rpc_messageFactory_init();

	size_t retVal = 0;

	for( size_t i = 0; i < (sizeof(msgPool)/sizeof(*msgPool)); i++ )
	{
		cxa_rpc_messageFactory_msgEntry_t* currEntry = (cxa_rpc_messageFactory_msgEntry_t*)&msgPool[i];
		if( currEntry->refCount == 0 ) retVal++;
	}

	return retVal;
}


cxa_rpc_message_t* cxa_rpc_messageFactory_getFreeMessage_empty(void)
{
	if( !isInit ) cxa_rpc_messageFactory_init();

	for( size_t i = 0; i < (sizeof(msgPool)/sizeof(*msgPool)); i++ )
	{
		cxa_rpc_messageFactory_msgEntry_t* currEntry = (cxa_rpc_messageFactory_msgEntry_t*)&msgPool[i];

		if( currEntry->refCount == 0 )
		{
			currEntry->refCount = 1;
			cxa_logger_trace(&logger, "message %p newly reserved", &currEntry->msg);

			cxa_fixedByteBuffer_clear(&currEntry->msgFbb);
			cxa_rpc_message_initEmpty(&currEntry->msg, &currEntry->msgFbb);
			return &currEntry->msg;
		}
	}

	cxa_logger_warn(&logger, "no free messages!");
	return NULL;
}


void cxa_rpc_messageFactory_incrementMessageRefCount(cxa_rpc_message_t *const msgIn)
{
	if( !isInit ) cxa_rpc_messageFactory_init();

	cxa_rpc_messageFactory_msgEntry_t* targetEntry = getMsgEntryFromMessage(msgIn);
	cxa_assert(targetEntry && (targetEntry->refCount < UINT8_MAX));

	targetEntry->refCount++;
	cxa_logger_trace(&logger, "message %p referenced", &currBuffer->msg);
}


void cxa_rpc_messageFactory_decrementMessageRefCount(cxa_rpc_message_t *const msgIn)
{
	if( !isInit ) cxa_rpc_messageFactory_init();

	cxa_rpc_messageFactory_msgEntry_t* targetEntry = getMsgEntryFromMessage(msgIn);
	cxa_assert(targetEntry);

	if( targetEntry->refCount > 0 )
	{
		targetEntry->refCount--;
		cxa_logger_trace(&logger, "message %p dereferenced", &targetEntry->msg);
	}
	else cxa_logger_warn(&logger, "mismatched decrement call for %p", &targetEntry->msg);
}


uint8_t cxa_rpc_messageFactory_getReferenceCountForMessage(cxa_rpc_message_t *const msgIn)
{
	if( !isInit ) cxa_rpc_messageFactory_init();

	cxa_rpc_messageFactory_msgEntry_t* targetEntry = getMsgEntryFromMessage(msgIn);
	cxa_assert(targetEntry);

	return targetEntry->refCount;
}


void cxa_rpc_messageFactory_writeStatusToFile(FILE* fileIn)
{
	cxa_assert(fileIn);

	fprintf(fileIn, "cxa_rpc_messageFactory status:" CXA_LINE_ENDING "{" CXA_LINE_ENDING);
	for( size_t i = 0; i < (sizeof(msgPool)/sizeof(*msgPool)); i++ )
	{
		cxa_rpc_messageFactory_msgEntry_t* currEntry = (cxa_rpc_messageFactory_msgEntry_t*)&msgPool[i];

		fprintf(fileIn, "   msg[%p],  buffer[%p],  refCount:%u,  size:%lu/%lu" CXA_LINE_ENDING,
				&currEntry->msg, &currEntry->msgFbb, currEntry->refCount,
				cxa_fixedByteBuffer_getSize_bytes(&currEntry->msgFbb) , cxa_fixedByteBuffer_getMaxSize_bytes(&currEntry->msgFbb));

	}
	fprintf(fileIn, "}" CXA_LINE_ENDING);
}


// ******** local function implementations ********
static cxa_rpc_messageFactory_msgEntry_t* getMsgEntryFromMessage(cxa_rpc_message_t *const msgIn)
{
	for( size_t i = 0; i < (sizeof(msgPool)/sizeof(*msgPool)); i++ )
	{
		cxa_rpc_messageFactory_msgEntry_t* currEntry = (cxa_rpc_messageFactory_msgEntry_t*)&msgPool[i];

		if( &currEntry->msg == msgIn ) return currEntry;
	}

	return NULL;
}
