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
#define ID_LEN_BYTES			(sizeof(CXA_RPC_ID_DATATYPE))


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
			if( !cxa_linkedField_initChild_fixedLen(&msgIn->id, &msgIn->src, ID_LEN_BYTES) ) { msgIn->areFieldsConfigured = false; return false; }

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
			if( !cxa_linkedField_initChild_fixedLen(&msgIn->id, &msgIn->dest, ID_LEN_BYTES) ) { msgIn->areFieldsConfigured = false; return false; }

			// next up, our source
			char* sourceString = (char*)cxa_fixedByteBuffer_get_pointerToIndex(msgIn->buffer, cxa_linkedField_getStartIndexOfNextField(&msgIn->id));
			if( sourceString == NULL ) { msgIn->areFieldsConfigured = false; return false; }
			if( !cxa_linkedField_initChild(&msgIn->src, &msgIn->id, strlen(sourceString)+1) ) { msgIn->areFieldsConfigured = false; return false; }

			// return value
			if( !cxa_linkedField_initChild_fixedLen(&msgIn->returnValue, &msgIn->src, 1) ) { msgIn->areFieldsConfigured = false; return false; }

			// finally, our params
			if( !cxa_linkedField_initChild(&msgIn->params, &msgIn->returnValue, (len_bytesIn - cxa_linkedField_getStartIndexOfNextField(&msgIn->returnValue))) ) { msgIn->areFieldsConfigured = false; return false; }

			break;
		}

		default:
			msgIn->areFieldsConfigured = false; return false;
	}

	return true;
}


bool cxa_rpc_message_initRequest(cxa_rpc_message_t *const msgIn, const char *const destIn, const char *const methodIn, uint8_t *const paramsIn, const size_t paramsSize_bytesIn)
{
	cxa_assert(msgIn);
	cxa_assert(destIn);
	cxa_assert(methodIn);

	// message should have been obtained through message factory, so the fbb should be good to go...
	cxa_assert(msgIn->buffer);

	// type
	if( !cxa_linkedField_initRoot_fixedLen(&msgIn->type, msgIn->buffer, 0, 1) || !cxa_linkedField_append_uint8(&msgIn->type, CXA_RPC_MESSAGE_TYPE_REQUEST) ) return false;

	// dest
	if( !cxa_linkedField_initChild(&msgIn->dest, &msgIn->type, 0) || !cxa_linkedField_append_cString(&msgIn->dest, destIn) ) return false;

	// method
	if( !cxa_linkedField_initChild(&msgIn->method, &msgIn->dest, 0) || !cxa_linkedField_append_cString(&msgIn->method, methodIn) ) return false;

	// source (empty for now)
	if( !cxa_linkedField_initChild(&msgIn->src, &msgIn->method, 0) ) return false;

	// id (0 for now)
	if( !cxa_linkedField_initChild_fixedLen(&msgIn->id, &msgIn->src, ID_LEN_BYTES) || !cxa_linkedField_append_uint16LE(&msgIn->id, 0) ) return false;

	// finally, the params
	if( !cxa_linkedField_initChild(&msgIn->params, &msgIn->id, 0) ) return false;
	if( (paramsIn != NULL) && (paramsSize_bytesIn > 0) && !cxa_linkedField_append(&msgIn->params, paramsIn, paramsSize_bytesIn) ) return false;

	// if we made it here, we're good to go!
	msgIn->areFieldsConfigured = true;
	return true;

}


bool cxa_rpc_message_initResponse(cxa_rpc_message_t *const msgIn, const char *const reqSrcIn, CXA_RPC_ID_DATATYPE reqIdIn)
{
	cxa_assert(msgIn);
	cxa_assert(reqSrcIn);

	// message should have been obtained through message factory, so the fbb should be good to go...
	cxa_assert(msgIn->buffer);

	// type
	if( !cxa_linkedField_initRoot_fixedLen(&msgIn->type, msgIn->buffer, 0, 1) || !cxa_linkedField_append_uint8(&msgIn->type, CXA_RPC_MESSAGE_TYPE_RESPONSE) ) return false;

	// dest
	if( !cxa_linkedField_initChild(&msgIn->dest, &msgIn->type, 0) || !cxa_linkedField_append_cString(&msgIn->dest, reqSrcIn) ) return false;

	// id
	if( !cxa_linkedField_initChild_fixedLen(&msgIn->id, &msgIn->dest, ID_LEN_BYTES) || !cxa_linkedField_append(&msgIn->id, (uint8_t*)&reqIdIn, sizeof(reqIdIn)) ) return false;

	// source (empty for now)
	if( !cxa_linkedField_initChild(&msgIn->src, &msgIn->id, 0) ) return false;

	// return value
	if( !cxa_linkedField_initChild_fixedLen(&msgIn->returnValue, &msgIn->src, 1) || !cxa_linkedField_append_uint8(&msgIn->returnValue, CXA_RPC_METHOD_RETVAL_UNKNOWN) ) return false;

	// finally, the params
	if( !cxa_linkedField_initChild(&msgIn->params, &msgIn->returnValue, 0) ) return false;

	// if we made it here, we're good to go!
	msgIn->areFieldsConfigured = true;
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


CXA_RPC_ID_DATATYPE cxa_rpc_message_getId(cxa_rpc_message_t *const msgIn)
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


bool cxa_rpc_message_getReturnValue(cxa_rpc_message_t *const msgIn)
{
	cxa_assert(msgIn);
	if( !msgIn->areFieldsConfigured ) return CXA_RPC_METHOD_RETVAL_UNKNOWN;

	if( cxa_rpc_message_getType(msgIn) != CXA_RPC_MESSAGE_TYPE_RESPONSE ) return CXA_RPC_METHOD_RETVAL_UNKNOWN;

	uint8_t retVal_raw = 0;
	return cxa_linkedField_get_uint8(&msgIn->returnValue, 0, retVal_raw) ? retVal_raw : CXA_RPC_METHOD_RETVAL_UNKNOWN;
}


bool cxa_rpc_message_setId(cxa_rpc_message_t *const msgIn, uint16_t idIn)
{
	cxa_assert(msgIn);
	if( !msgIn->areFieldsConfigured ) return false;

	return cxa_linkedField_replace_uint16LE(&msgIn->id, 0, idIn);
}


bool cxa_rpc_message_setReturnValue(cxa_rpc_message_t *const msgIn, cxa_rpc_method_retVal_t returnValueIn)
{
	cxa_assert(msgIn);
	if( !msgIn->areFieldsConfigured ) return false;

	return cxa_linkedField_replace_uint8(&msgIn->returnValue, 0, returnValueIn);
}


bool cxa_rpc_message_prependNodeNameToSource(cxa_rpc_message_t *const msgIn, const char *const nodeNameToPrepend)
{
	cxa_assert(msgIn);
	cxa_assert(nodeNameToPrepend);
	if( !msgIn->areFieldsConfigured ) return false;

	bool wasSourceEmpty = (cxa_linkedField_getSize_bytes(&msgIn->src) == 0);

	if( !cxa_linkedField_insert_cString(&msgIn->src, 0, nodeNameToPrepend) ) return false;
	if( !wasSourceEmpty && (strcmp(CXA_RPC_PATH_GLOBAL_ROOT, CXA_RPC_PATH_SEP) == 0) && (strcmp(nodeNameToPrepend, CXA_RPC_PATH_GLOBAL_ROOT) != 0) )
	{
		// add our path separator
		return cxa_linkedField_replace(&msgIn->src, strlen(nodeNameToPrepend), (uint8_t*)CXA_RPC_PATH_SEP, strlen(CXA_RPC_PATH_SEP));
	}
	else if( !wasSourceEmpty && (strcmp(CXA_RPC_PATH_GLOBAL_ROOT, CXA_RPC_PATH_SEP) == 0) && (strcmp(nodeNameToPrepend, CXA_RPC_PATH_GLOBAL_ROOT) == 0) )
	{
		return cxa_linkedField_remove_uint8(&msgIn->src, strlen(nodeNameToPrepend));
	}
	return true;
}


bool cxa_rpc_message_destination_getFirstPathComponent(cxa_rpc_message_t *const msgIn, char** pathCompOut, size_t* pathCompLen_bytesOut)
{
	cxa_assert(msgIn);
	if( !msgIn->areFieldsConfigured ) return false;

	// get our destination
	char* dest = (char*)cxa_linkedField_get_pointerToIndex(&msgIn->dest, 0);
	if( (dest == NULL) || (dest[0] == 0) ) return false;
	size_t destLen_bytes = strlen(dest);

	// look for our path separator
	char* firstPathSep = strstr(dest, CXA_RPC_PATH_SEP);
	if( firstPathSep == NULL )
	{
		// this must be our last path component
		if( pathCompOut ) *pathCompOut = dest;
		if( pathCompLen_bytesOut ) *pathCompLen_bytesOut = destLen_bytes;
		return true;
	}

	// if we made it here, we found a path separator
	size_t compLen_bytes = firstPathSep - dest;
	if( compLen_bytes == 0 ) compLen_bytes = 1;

	if( pathCompOut ) *pathCompOut = dest;
	if( pathCompLen_bytesOut ) *pathCompLen_bytesOut = compLen_bytes;
	return true;
}


bool cxa_rpc_message_destination_removeFirstPathComponent(cxa_rpc_message_t *const msgIn)
{
	cxa_assert(msgIn);
	if( !msgIn->areFieldsConfigured ) return false;

	char *pathComp = NULL;
	size_t firstCompLen_bytes = 0;
	if( !cxa_rpc_message_destination_getFirstPathComponent(msgIn, &pathComp, &firstCompLen_bytes) ) return false;
	if( (pathComp == NULL) || (firstCompLen_bytes == 0) ) return true;

	// see if there are additional components in the destination path
	size_t destSize_bytes = cxa_linkedField_getSize_bytes(&msgIn->dest);
	if( destSize_bytes == 0 ) return false;

	// adjust for null terminator
	destSize_bytes--;
	if( (firstCompLen_bytes == destSize_bytes) || (strncmp(pathComp, CXA_RPC_PATH_GLOBAL_ROOT, strlen(CXA_RPC_PATH_GLOBAL_ROOT)) == 0))
	{
		// this is either the last component OR a GLOBALROOT component
		return cxa_linkedField_remove(&msgIn->dest, 0, firstCompLen_bytes);
	}
	else if( firstCompLen_bytes < destSize_bytes )
	{
		// this is NOT the last component, verify and remove path separator
		if( strncmp(&pathComp[firstCompLen_bytes], CXA_RPC_PATH_SEP, strlen(CXA_RPC_PATH_SEP)) != 0 ) return false;
		// +1 is for the path separator
		return cxa_linkedField_remove(&msgIn->dest, 0, firstCompLen_bytes+1);
	}

	// if we made it here, something messed up
	return false;
}


char* cxa_rpc_message_getFriendlyTypeString(cxa_rpc_message_type_t typeIn)
{
	char *retVal = NULL;
	switch( typeIn )
	{
		case CXA_RPC_MESSAGE_TYPE_REQUEST:
			retVal = "request";
			break;

		case CXA_RPC_MESSAGE_TYPE_RESPONSE:
			retVal = "response";
			break;

		default:
			retVal = "unknown";
			break;
	}

	return retVal;
}


// ******** local function implementations ********
