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
#include "cxa_rpc_message.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdio.h>
#include <string.h>
#include <cxa_assert.h>


// ******** local macro definitions ********
#define INDEX_MSG_TYPE			0
#define INDEX_MSG_DEST			1
#define ID_LEN_BYTES			2


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_rpc_message_initEmpty(cxa_rpc_message_t *const msgIn, cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(msgIn);
	cxa_assert(fbbIn);

	// setup our internal state
	msgIn->buffer = fbbIn;
	msgIn->areFieldsConfigured = false;
}


bool cxa_rpc_message_validateReceivedBytes(cxa_rpc_message_t *const msgIn, const size_t startingIndexIn, const size_t len_bytesIn)
{
	cxa_assert(msgIn);

	// we need to set this temporarily so we can parse our fields as we go
	msgIn->areFieldsConfigured = true;

	// setup our linkedFields
	if( !cxa_linkedField_initRoot_fixedLen(&msgIn->type, msgIn->buffer, startingIndexIn, 1) ) { msgIn->areFieldsConfigured = false; return false; }

	// check our message type
	cxa_rpc_message_type_t msgType = cxa_rpc_message_getType(msgIn);
	switch( msgType )
	{
		case CXA_RPC_MESSAGE_TYPE_REQUEST:
		{
			// next up, our destination
			char* destString = (char*)cxa_fixedByteBuffer_get_pointerToIndex(msgIn->buffer, cxa_linkedField_getStartIndexOfNextField(&msgIn->type));
			if( destString == NULL ) { msgIn->areFieldsConfigured = false; return false; }
			if( !cxa_linkedField_initChild(&msgIn->dest, &msgIn->type, strlen(destString)+1) ) { msgIn->areFieldsConfigured = false; return false; }

			// next up, our method
			char* methodString = (char*)cxa_fixedByteBuffer_get_pointerToIndex(msgIn->buffer, cxa_linkedField_getStartIndexOfNextField(&msgIn->dest));
			if( methodString == NULL ) { msgIn->areFieldsConfigured = false; return false; }
			if( !cxa_linkedField_initChild(&msgIn->method, &msgIn->dest, strlen(methodString)+1) ) { msgIn->areFieldsConfigured = false; return false; }

			// next up, our source
			char* sourceString = (char*)cxa_fixedByteBuffer_get_pointerToIndex(msgIn->buffer, cxa_linkedField_getStartIndexOfNextField(&msgIn->method));
			if( sourceString == NULL ) { msgIn->areFieldsConfigured = false; return false; }
			if( !cxa_linkedField_initChild(&msgIn->src, &msgIn->method, strlen(sourceString)+1) ) { msgIn->areFieldsConfigured = false; return false; }

			// next up, our id
			if( !cxa_linkedField_initChild_fixedLen(&msgIn->id, &msgIn->src, 2) ) { msgIn->areFieldsConfigured = false; return false; }

			// finally, our params
			if( !cxa_linkedField_initChild(&msgIn->params, &msgIn->id, (len_bytesIn - cxa_linkedField_getStartIndexOfNextField(&msgIn->id))) ) { msgIn->areFieldsConfigured = false; return false; }

			break;
		}

		case CXA_RPC_MESSAGE_TYPE_RESPONSE:
		{
			// next up, our destination
			char* destString = (char*)cxa_fixedByteBuffer_get_pointerToIndex(msgIn->buffer, cxa_linkedField_getStartIndexOfNextField(&msgIn->type));
			if( destString == NULL ) { msgIn->areFieldsConfigured = false; return false; }
			if( !cxa_linkedField_initChild(&msgIn->dest, &msgIn->type, strlen(destString)+1) ) { msgIn->areFieldsConfigured = false; return false; }

			// next up, our id
			if( !cxa_linkedField_initChild_fixedLen(&msgIn->id, &msgIn->dest, 2) ) { msgIn->areFieldsConfigured = false; return false; }

			// next up, our source
			char* sourceString = (char*)cxa_fixedByteBuffer_get_pointerToIndex(msgIn->buffer, cxa_linkedField_getStartIndexOfNextField(&msgIn->id));
			if( sourceString == NULL ) { msgIn->areFieldsConfigured = false; return false; }
			if( !cxa_linkedField_initChild(&msgIn->src, &msgIn->id, strlen(sourceString)+1) ) { msgIn->areFieldsConfigured = false; return false; }

			// finally, our params
			if( !cxa_linkedField_initChild(&msgIn->params, &msgIn->src, (len_bytesIn - cxa_linkedField_getStartIndexOfNextField(&msgIn->src))) ) { msgIn->areFieldsConfigured = false; return false; }

			break;
		}

		default:
			msgIn->areFieldsConfigured = false; return false;
	}

	return true;
}


cxa_rpc_message_type_t cxa_rpc_message_getType(cxa_rpc_message_t *const msgIn)
{
	cxa_assert(msgIn);
	if( !msgIn->areFieldsConfigured ) return CXA_RPC_MESSAGE_TYPE_UNKNOWN;

	uint8_t msgType_raw;
	if( !cxa_linkedField_get_uint8(&msgIn->type, 0, msgType_raw) ) return CXA_RPC_MESSAGE_TYPE_UNKNOWN;

	cxa_rpc_message_type_t retVal = CXA_RPC_MESSAGE_TYPE_UNKNOWN;
	switch( (cxa_rpc_message_type_t)msgType_raw )
	{
		case CXA_RPC_MESSAGE_TYPE_REQUEST:
		case CXA_RPC_MESSAGE_TYPE_RESPONSE:
			retVal = (cxa_rpc_message_type_t)msgType_raw;
			break;

		default: break;
	}

	return retVal;
}


char* cxa_rpc_message_getDestination(cxa_rpc_message_t *const msgIn)
{
	cxa_assert(msgIn);
	if( !msgIn->areFieldsConfigured ) return NULL;

	return (char*)cxa_linkedField_get_pointerToIndex(&msgIn->dest, 0);
}


char* cxa_rpc_message_getMethod(cxa_rpc_message_t *const msgIn)
{
	cxa_assert(msgIn);
	if( !msgIn->areFieldsConfigured ) return NULL;

	cxa_rpc_message_type_t msgType = cxa_rpc_message_getType(msgIn);
	if( msgType != CXA_RPC_MESSAGE_TYPE_REQUEST ) return NULL;

	return (char*)cxa_linkedField_get_pointerToIndex(&msgIn->method, 0);
}


char* cxa_rpc_message_getSource(cxa_rpc_message_t *const msgIn)
{
	cxa_assert(msgIn);
	if( !msgIn->areFieldsConfigured ) return NULL;

	return (char*)cxa_linkedField_get_pointerToIndex(&msgIn->src, 0);
}


uint16_t cxa_rpc_message_getId(cxa_rpc_message_t *const msgIn)
{
	cxa_assert(msgIn);
	if( !msgIn->areFieldsConfigured ) return 0;

	uint16_t idOut;
	return (cxa_linkedField_get_uint16LE(&msgIn->id, 0, idOut)) ? idOut : 0;
}


cxa_linkedField_t* cxa_rpc_message_getParams(cxa_rpc_message_t *const msgIn)
{
	cxa_assert(msgIn);
	if( !msgIn->areFieldsConfigured ) return NULL;

	return &msgIn->params;
}


// ******** local function implementations ********
