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
#include "cxa_mqtt_messageFactory.h"


// ******** includes ********
#include <stddef.h>
#include <cxa_array.h>
#include <cxa_assert.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_INFO
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********
typedef struct
{
	uint8_t refCount;

	cxa_mqtt_message_t msg;

	cxa_fixedByteBuffer_t msgFbb;
	uint8_t msgBuffer_raw[CXA_MQTT_MESSAGEFACTORY_MESSAGE_SIZE_BYTES];
}messageEntry_t;


// ******** local function prototypes ********
static void initIfNeeded(void);
static messageEntry_t* getMsgEntryFromMessage(cxa_mqtt_message_t *const msgIn);


// ********  local variable declarations *********
static bool isInit = false;

static cxa_array_t msgEntries;
static messageEntry_t msgEntries_raw[CXA_MQTT_MESSAGEFACTORY_NUM_MESSAGES];

static cxa_logger_t logger;


// ******** global function implementations ********
size_t cxa_mqtt_messageFactory_getNumFreeMessages(void)
{
	initIfNeeded();

	size_t numFreeMessages = 0;
	cxa_array_iterate(&msgEntries, currEntry, messageEntry_t)
	{
		if( currEntry == NULL ) continue;

		if( currEntry->refCount == 0 ) numFreeMessages++;
	}

	return numFreeMessages;
}


cxa_mqtt_message_t* cxa_mqtt_messageFactory_getFreeMessage_empty(void)
{
	initIfNeeded();

	cxa_array_iterate(&msgEntries, currEntry, messageEntry_t)
	{
		if( currEntry == NULL) continue;

		if( currEntry->refCount == 0 )
		{
			currEntry->refCount = 1;
			cxa_logger_trace(&logger, "message %p newly reserved", &currEntry->msg);

			cxa_fixedByteBuffer_clear(&currEntry->msgFbb);
			cxa_mqtt_message_initEmpty(&currEntry->msg, &currEntry->msgFbb);
			return &currEntry->msg;
		}
	}

	cxa_logger_warn(&logger, "no free messages!");
	return NULL;
}


cxa_mqtt_message_t* cxa_mqtt_messageFactory_getMessage_byBuffer(cxa_fixedByteBuffer_t *const fbbIn)
{
	initIfNeeded();

	// simple case (better than an assert in this case)
	if( fbbIn == NULL) return NULL;

	cxa_array_iterate(&msgEntries, currEntry, messageEntry_t)
	{
		if( currEntry == NULL) continue;

		if( (currEntry->refCount != 0) && (&currEntry->msgFbb == fbbIn) ) return &currEntry->msg;
	}

	// if we made it here, we couldn't find a match
	return NULL;
}


void cxa_mqtt_messageFactory_incrementMessageRefCount(cxa_mqtt_message_t *const msgIn)
{
	initIfNeeded();

	messageEntry_t* targetEntry = getMsgEntryFromMessage(msgIn);
	cxa_assert( targetEntry && (targetEntry->refCount < UINT8_MAX) );

	targetEntry->refCount++;
	cxa_logger_trace(&logger, "message %p referenced (%d)", &targetEntry->msg, targetEntry->refCount);
}


void cxa_mqtt_messageFactory_decrementMessageRefCount(cxa_mqtt_message_t *const msgIn)
{
	initIfNeeded();

	messageEntry_t* targetEntry = getMsgEntryFromMessage(msgIn);
	cxa_assert(targetEntry);

	if( targetEntry->refCount > 0 )
	{
		targetEntry->refCount--;
		cxa_logger_trace(&logger, "message %p dereferenced (%d)", &targetEntry->msg, targetEntry->refCount);
	}
	else cxa_logger_warn(&logger, "mismatched decrement call for %p", &targetEntry->msg);
}


uint8_t cxa_mqtt_messageFactory_getReferenceCountForMessage(cxa_mqtt_message_t *const msgIn)
{
	initIfNeeded();

	messageEntry_t* targetEntry = getMsgEntryFromMessage(msgIn);
	cxa_assert(targetEntry);

	return targetEntry->refCount;
}


// ******** local function implementations ********
static void initIfNeeded(void)
{
	if( isInit ) return;

	// initialize our logger
	cxa_logger_init(&logger, "mqttMsgFactory");

	// initialize our messages
	cxa_array_init_inPlace(&msgEntries, sizeof(*msgEntries_raw), (sizeof(msgEntries_raw)/sizeof(*msgEntries_raw)), (void*)msgEntries_raw, sizeof(msgEntries_raw));
	cxa_array_iterate(&msgEntries, currEntry, messageEntry_t)
	{
		cxa_assert(currEntry);
		cxa_fixedByteBuffer_initStd(&currEntry->msgFbb, currEntry->msgBuffer_raw);

		currEntry->refCount = 0;
	}


	isInit = true;
}


static messageEntry_t* getMsgEntryFromMessage(cxa_mqtt_message_t *const msgIn)
{
	cxa_array_iterate(&msgEntries, currEntry, messageEntry_t)
	{
		if( currEntry == NULL ) continue;

		if( &currEntry->msg == msgIn ) return currEntry;
	}

	return NULL;
}
