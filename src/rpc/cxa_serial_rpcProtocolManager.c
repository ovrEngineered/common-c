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
#include "cxa_serial_rpcProtocolManager.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdio.h>
#include <cxa_assert.h>
#include <cxa_timeDiff.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_DEBUG
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define RPC_PROTOCOL_VERSION				1

#define RPC_MSGTYPE_REQUEST					1
#define RPC_MSGTYPE_RESPONSE				2
#define RPC_MSGTYPE_NOTIFICATION			3

#define DEFAULT_SYNC_TIMEOUT_MS				1000


// ******** local type definitions ********


// ******** local function prototypes ********
static cxa_fixedByteBuffer_t* usedMsgMap_getParentBuffer_fromDataByteBuffer(cxa_serial_rpcProtocolManager_t *const srpmIn, cxa_fixedByteBuffer_t *dataByteBufferIn);
static void messageRx_cb(cxa_fixedByteBuffer_t *const dataBytesIn, void *const userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_serial_rpcProtocolManager_init(cxa_serial_rpcProtocolManager_t *const srpmIn, cxa_timeBase_t *const tbIn)
{
	cxa_assert(srpmIn);
	
	// save our references
	srpmIn->timeBase = tbIn;
	
	// set some sensible defaults
	srpmIn->nextId = 0;
	srpmIn->unknownRequestListener.cb = NULL;
	srpmIn->unknownRequestListener.userVar = NULL;
	srpmIn->unknownNotificationListener.cb = NULL;
	srpmIn->unknownNotificationListener.userVar = NULL;
	
	// setup our used message map
	cxa_array_init(&srpmIn->usedMessageMap, sizeof(*srpmIn->usedMessageMap_raw), (void*)srpmIn->usedMessageMap_raw, sizeof(srpmIn->usedMessageMap_raw));
	
	// setup our protocol parser
	cxa_serial_protocolParser_init(&srpmIn->protoParser, RPC_PROTOCOL_VERSION);
	
	// setup our logger
	cxa_logger_init(&srpmIn->logger, "rpcProtoMan");
	
	// setup our arrays/maps
	cxa_map_init(&srpmIn->inflightRequests, sizeof(uint16_t), sizeof(cxa_serial_rpcProtocolManager_inflightRequest_t), (void*)srpmIn->inflightRequests_raw, sizeof(srpmIn->inflightRequests_raw));
	cxa_map_init(&srpmIn->requestListeners, sizeof(uint8_t), sizeof(cxa_serial_rpcProtocolManager_requestListener_entry_t), (void*)srpmIn->requestListeners_raw, sizeof(srpmIn->requestListeners_raw));
	cxa_map_init(&srpmIn->notificationListeners, sizeof(uint8_t), sizeof(cxa_serial_rpcProtocolManager_notificationListener_entry_t), (void*)srpmIn->notificationListeners_raw, sizeof(srpmIn->notificationListeners_raw));
	cxa_array_init(&srpmIn->usedMessageMap, sizeof(*srpmIn->usedMessageMap_raw), (void*)srpmIn->usedMessageMap_raw, sizeof(srpmIn->usedMessageMap_raw));
	
	// register to receive message from our protocol parser
	cxa_serial_protocolParser_addMessageListener(&srpmIn->protoParser, messageRx_cb, (void*)srpmIn);
}


void cxa_serial_rpcProtocolManager_start(cxa_serial_rpcProtocolManager_t *const srpmIn, FILE *const fdIn)
{
	cxa_assert(srpmIn);
	
	cxa_serial_protocolParser_setSerialDevice(&srpmIn->protoParser, fdIn);
}


void cxa_serial_rpcProtocolManager_stop(cxa_serial_rpcProtocolManager_t *const srpmIn)
{
	
	cxa_assert(srpmIn);
	
	cxa_serial_protocolParser_setSerialDevice(&srpmIn->protoParser, NULL);
}


cxa_fixedByteBuffer_t* cxa_serial_rpcProtocolManager_sendRequest_getFreeBuffer(cxa_serial_rpcProtocolManager_t *const srpmIn, uint8_t opCodeIn)
{
	cxa_assert(srpmIn);
	
	// reserve a message from the protocol parser
	cxa_fixedByteBuffer_t *rpcMsgBuffer = cxa_serial_protocolParser_reserveFreeBuffer(&srpmIn->protoParser);
	if( rpcMsgBuffer == NULL ) return NULL;
	
	// add the rpc header to the buffer
	cxa_fixedByteBuffer_append_byte(rpcMsgBuffer, RPC_MSGTYPE_REQUEST);
	uint16_t id = srpmIn->nextId++;
	cxa_fixedByteBuffer_append_uint16LE(rpcMsgBuffer, id);
	cxa_fixedByteBuffer_append_byte(rpcMsgBuffer, opCodeIn);
	
	// add it to our map so we know which messages we are controlling
	cxa_serial_rpcProtocolManager_usedMsgMap_entry_t *msgMapEntry = (cxa_serial_rpcProtocolManager_usedMsgMap_entry_t*)cxa_array_append_empty(&srpmIn->usedMessageMap);
	cxa_assert(msgMapEntry != NULL);
	msgMapEntry->parentBuffer = rpcMsgBuffer;
	cxa_fixedByteBuffer_init_subsetOfCapacity(&msgMapEntry->dataBytes, rpcMsgBuffer, 4, CXA_FIXED_BYTE_BUFFER_LEN_ALL);
	
	return &msgMapEntry->dataBytes;
}


cxa_fixedByteBuffer_t* cxa_serial_rpcProtocolManager_sendRequest_sync(cxa_serial_rpcProtocolManager_t *const srpmIn, cxa_fixedByteBuffer_t *const dataBytesIn)
{
	cxa_assert(srpmIn);
	cxa_assert(dataBytesIn);
	
	// make sure it was our message to begin with...
	cxa_fixedByteBuffer_t *rpcMessage = usedMsgMap_getParentBuffer_fromDataByteBuffer(srpmIn, dataBytesIn);
	cxa_assert(rpcMessage != NULL);
	
	uint16_t id = cxa_fixedByteBuffer_get_uint16LE(rpcMessage, 1);
	
	// setup our inflight request
	cxa_serial_rpcProtocolManager_inflightRequest_t *ifl = cxa_map_put_empty(&srpmIn->inflightRequests, &id);
	ifl->responseReceived = false;
	ifl->responseBytes = NULL;
	
	// now send the buffer and free the request (we'll reserve a response later)
	if( !cxa_serial_protocolParser_writeMessage(&srpmIn->protoParser, rpcMessage) )
	{
		// message send failed
		cxa_logger_error(&srpmIn->logger, "failed to send request");
		
		cxa_map_removeElement(&srpmIn->inflightRequests, &id);
		cxa_serial_protocolParser_freeReservedBuffer(&srpmIn->protoParser, rpcMessage);
		return NULL;
	}
	cxa_serial_protocolParser_freeReservedBuffer(&srpmIn->protoParser, rpcMessage);
	
	// message was sent successfully...now wait for our response (or a timeout)
	cxa_timeDiff_t td_rxTimeout;
	cxa_timeDiff_init(&td_rxTimeout, srpmIn->timeBase);
	while( !cxa_timeDiff_isElapsed_ms(&td_rxTimeout, DEFAULT_SYNC_TIMEOUT_MS) && !ifl->responseReceived ) {}
		
	// one way or another, we're done waiting for the response...
	// get our request and return the buffer
	cxa_fixedByteBuffer_t *retVal = ifl->responseBytes;
	cxa_map_removeElement(&srpmIn->inflightRequests, &id);
	return retVal;
}


void cxa_serial_rpcProtocolManager_sendRequest_returnResponseBuffer(cxa_serial_rpcProtocolManager_t *const srpmIn, cxa_fixedByteBuffer_t *const dataBytesIn)
{
	cxa_assert(srpmIn);
	
	cxa_fixedByteBuffer_t *rpcMessage = usedMsgMap_getParentBuffer_fromDataByteBuffer(srpmIn, dataBytesIn);
	if( rpcMessage != NULL ) cxa_serial_protocolParser_freeReservedBuffer(&srpmIn->protoParser, rpcMessage);
}


cxa_fixedByteBuffer_t* cxa_serial_rpcProtocolManager_sendNotification_getBuffer(cxa_serial_rpcProtocolManager_t *const srpmIn, uint8_t opCodeIn)
{
	cxa_assert(srpmIn);
	
	// reserve a message from the protocol parser
	cxa_fixedByteBuffer_t *rpcMsgBuffer = cxa_serial_protocolParser_reserveFreeBuffer(&srpmIn->protoParser);
	if( rpcMsgBuffer == NULL ) return NULL;
	
	// add the rpc header to the buffer
	cxa_fixedByteBuffer_append_byte(rpcMsgBuffer, RPC_MSGTYPE_NOTIFICATION);
	cxa_fixedByteBuffer_append_byte(rpcMsgBuffer, opCodeIn);
	
	// add it to our map so we know which messages we are controlling
	cxa_fixedByteBuffer_t *dataBytesBuffer = (cxa_fixedByteBuffer_t*)cxa_array_append_empty(&srpmIn->usedMessageMap);
	cxa_assert(dataBytesBuffer != NULL);
	cxa_fixedByteBuffer_init_subsetOfCapacity(dataBytesBuffer, rpcMsgBuffer, 2, CXA_FIXED_BYTE_BUFFER_LEN_ALL);
	
	return dataBytesBuffer;
}


void cxa_serial_rpcProtocolManager_sendNotification(cxa_serial_rpcProtocolManager_t *const srpmIn, cxa_fixedByteBuffer_t *const dataBytesIn)
{
	cxa_assert(srpmIn);
	cxa_assert(dataBytesIn);
	
	// make sure it was our message to begin with...
	cxa_fixedByteBuffer_t *rpcMessage = usedMsgMap_getParentBuffer_fromDataByteBuffer(srpmIn, dataBytesIn);
	cxa_assert(rpcMessage != NULL);
	
	// now send the buffer and free the notification
	if( !cxa_serial_protocolParser_writeMessage(&srpmIn->protoParser, rpcMessage) )
	{
		// message send failed
		cxa_logger_error(&srpmIn->logger, "failed to send notification");
	}
	
	cxa_serial_protocolParser_freeReservedBuffer(&srpmIn->protoParser, rpcMessage);
}


void cxa_serial_rpcProtocolManager_addRequestListener(cxa_serial_rpcProtocolManager_t *const srpmIn, uint8_t opCodeIn, cxa_serial_rpcProtocolManager_cb_requestReceived_t cbIn, void *const userVarIn)
{
	cxa_assert(srpmIn);
	cxa_assert(cbIn);
	
	// create our new entry and add it to our map
	cxa_serial_rpcProtocolManager_requestListener_entry_t newEntry = {.cb=cbIn, .userVar=userVarIn};
	cxa_assert( cxa_map_put(&srpmIn->requestListeners, (void*)&opCodeIn, (void*)&newEntry) );
}


void cxa_serial_rpcProtocolManager_addNotificationListener(cxa_serial_rpcProtocolManager_t *const srpmIn, uint8_t opCodeIn, cxa_serial_rpcProtocolManager_cb_notificationReceived_t cbIn, void *const userVarIn)
{
	cxa_assert(srpmIn);
	cxa_assert(cbIn);
	
	// create our new entry and add it to our map
	cxa_serial_rpcProtocolManager_notificationListener_entry_t newEntry = {.cb=cbIn, .userVar=userVarIn};
	cxa_assert( cxa_map_put(&srpmIn->notificationListeners, (void*)&opCodeIn, (void*)&newEntry) );
}


void cxa_serial_rpcProtocolManager_setUnknownRequestListener(cxa_serial_rpcProtocolManager_t *const srpmIn, cxa_serial_rpcProtocolManager_cb_requestReceived_t cbIn, void *const userVarIn)
{
	cxa_assert(srpmIn);
	cxa_assert(cbIn);

	srpmIn->unknownRequestListener.cb = cbIn;
	srpmIn->unknownRequestListener.userVar = userVarIn;
}


void cxa_serial_rpcProtocolManager_addUnknownNotificationListener(cxa_serial_rpcProtocolManager_t *const srpmIn, cxa_serial_rpcProtocolManager_cb_notificationReceived_t cbIn, void *const userVarIn)
{
	cxa_assert(srpmIn);
	cxa_assert(cbIn);

	srpmIn->unknownNotificationListener.cb = cbIn;
	srpmIn->unknownNotificationListener.userVar = userVarIn;
}


void cxa_serial_rpcProtocolManager_update(cxa_serial_rpcProtocolManager_t *const srpmIn)
{
	cxa_assert(srpmIn);
	
	cxa_serial_protocolParser_update(&srpmIn->protoParser);
}


// ******** local function implementations ********
static cxa_fixedByteBuffer_t* usedMsgMap_getParentBuffer_fromDataByteBuffer(cxa_serial_rpcProtocolManager_t *const srpmIn, cxa_fixedByteBuffer_t *dataByteBufferIn)
{
	cxa_assert(dataByteBufferIn);
	if( dataByteBufferIn == NULL ) return NULL;
	
	for(size_t i = 0; i < cxa_array_getSize_elems(&srpmIn->usedMessageMap); i++ )
	{
		cxa_serial_rpcProtocolManager_usedMsgMap_entry_t *currEntry = (cxa_serial_rpcProtocolManager_usedMsgMap_entry_t*)cxa_array_getAtIndex(&srpmIn->usedMessageMap, i);
		if( currEntry == NULL ) continue;
		
		if( &currEntry->dataBytes == dataByteBufferIn ) return currEntry->parentBuffer;
	}
	
	return NULL;
}


static void messageRx_cb(cxa_fixedByteBuffer_t *const dataBytesIn, void *const userVarIn)
{
	cxa_serial_rpcProtocolManager_t *const srpmIn = (cxa_serial_rpcProtocolManager_t *const)userVarIn;
	cxa_assert(srpmIn);
	
	size_t dataBytesSize = 0;
	if( (dataBytesIn == NULL) || ((dataBytesSize = cxa_fixedByteBuffer_getCurrSize(dataBytesIn)) < 2) ) return;
	
	// first, we need to see what kind of message this is
	uint8_t msgType = cxa_fixedByteBuffer_get_byte(dataBytesIn, 0);
	if( msgType == RPC_MSGTYPE_REQUEST )
	{
		if( dataBytesSize < 4 )
		{
			cxa_logger_debug(&srpmIn->logger, "malformed request: < 4 bytes");
			return;
		}
		
		// get our opcode and see if we have a subscribed listener
		uint8_t opCode = cxa_fixedByteBuffer_get_byte(dataBytesIn, 3);
		uint16_t id = cxa_fixedByteBuffer_get_uint16LE(dataBytesIn, 1);
		cxa_logger_trace(&srpmIn->logger, "request received  opCode: 0x%02X  id: 0x%04X", opCode, id);
		
		// make sure we actually have someone interested in this data...
		cxa_serial_rpcProtocolManager_requestListener_entry_t *rl = (cxa_serial_rpcProtocolManager_requestListener_entry_t*)cxa_map_get(&srpmIn->requestListeners, &opCode);
		if( ((rl != NULL) && (rl->cb != NULL)) || (srpmIn->unknownRequestListener.cb != NULL) )
		{
			// setup our request data (subset of our received data)
			cxa_fixedByteBuffer_t requestBytes;
			cxa_fixedByteBuffer_init_subsetOfData(&requestBytes, dataBytesIn, 4, CXA_FIXED_BYTE_BUFFER_LEN_ALL);
		
			// setup our response message and data buffer
			cxa_fixedByteBuffer_t *rpcMessage = cxa_serial_protocolParser_reserveFreeBuffer(&srpmIn->protoParser);
			if( rpcMessage == NULL )
			{
				cxa_logger_warn(&srpmIn->logger, "unable to reserve message buffer for response...dropping request");
				return;
			}
			cxa_fixedByteBuffer_append_byte(rpcMessage, RPC_MSGTYPE_RESPONSE);
			cxa_fixedByteBuffer_append_uint16LE(rpcMessage, id);
		
			cxa_fixedByteBuffer_t responseBytes;
			cxa_fixedByteBuffer_init_subsetOfCapacity(&responseBytes, rpcMessage, 3, CXA_FIXED_BYTE_BUFFER_LEN_ALL);
		
			bool shouldSendResponse = false;
			cxa_serial_rpcProtocolManager_requestListener_entry_t *rl = (cxa_serial_rpcProtocolManager_requestListener_entry_t*)cxa_map_get(&srpmIn->requestListeners, &opCode);
			if( (rl == NULL) && (srpmIn->unknownRequestListener.cb != NULL) )
			{
				cxa_logger_debug(&srpmIn->logger, "unexpected request opCode[0x%02X]...sending to url", opCode);
				shouldSendResponse = srpmIn->unknownRequestListener.cb(&requestBytes, &responseBytes, srpmIn->unknownRequestListener.userVar);
			}			
			else if( rl->cb != NULL )
			{
				cxa_logger_trace(&srpmIn->logger, "calling registered callback @ %p", rl->cb);
				shouldSendResponse = rl->cb(&requestBytes, &responseBytes, rl->userVar);
			}			
		
			// if we have a response...send it
			if( shouldSendResponse )
			{	
				cxa_logger_trace(&srpmIn->logger, "response requested...sending");
				// send the response
				if( !cxa_serial_protocolParser_writeMessage(&srpmIn->protoParser, rpcMessage) )
				{
					cxa_logger_error(&srpmIn->logger, "failed to send response");
				}
			}
		
			// free our response buffer
			cxa_serial_protocolParser_freeReservedBuffer(&srpmIn->protoParser, rpcMessage);
		}else cxa_logger_debug(&srpmIn->logger, "received unexpected request opCode[0x%02X]", opCode);
	}
	else if( msgType == RPC_MSGTYPE_RESPONSE )
	{
		if( dataBytesSize < 3 )
		{
			cxa_logger_debug(&srpmIn->logger, "malformed response: < 4 bytes");
			return;
		}
		
		// we have a valid response...get our id and see if it is
		// a response to an inflight request
		uint16_t id = cxa_fixedByteBuffer_get_uint16LE(dataBytesIn, 1);
		cxa_logger_trace(&srpmIn->logger, "response received, id: 0x%04X", id);
		
		cxa_serial_rpcProtocolManager_inflightRequest_t *ifr = (cxa_serial_rpcProtocolManager_inflightRequest_t*)cxa_map_get(&srpmIn->inflightRequests, &id);
		if( ifr == NULL )
		{
			cxa_logger_debug(&srpmIn->logger, "received unexpected response id[0x%04X]", id);
			return;
		}
		
		// if we made it here, it _is_ a response we're waiting for...
		// reserve the message from the protocol parser...
		if( !cxa_serial_protocolParser_reserveExistingBuffer(&srpmIn->protoParser, dataBytesIn) )
		{
			cxa_logger_warn(&srpmIn->logger, "unable to reserve message buffer for response handling...dropping response");
			return;
		}		
		
		// add it to our map so we know which messages we are controlling
		cxa_serial_rpcProtocolManager_usedMsgMap_entry_t *msgMapEntry = (cxa_serial_rpcProtocolManager_usedMsgMap_entry_t*)cxa_array_append_empty(&srpmIn->usedMessageMap);
		cxa_assert(msgMapEntry != NULL);
		msgMapEntry->parentBuffer = dataBytesIn;
		cxa_fixedByteBuffer_init_subsetOfData(&msgMapEntry->dataBytes, dataBytesIn, 3, CXA_FIXED_BYTE_BUFFER_LEN_ALL);
		
		ifr->responseBytes = &msgMapEntry->dataBytes;
		ifr->responseReceived = true;
	}
	else if( msgType == RPC_MSGTYPE_NOTIFICATION )
	{
		// get our opcode and see if we have a subscribed listener
		uint8_t opCode = cxa_fixedByteBuffer_get_byte(dataBytesIn, 1);
		cxa_logger_trace(&srpmIn->logger, "notification received, opCode: 0x%02X", opCode);
		
		cxa_fixedByteBuffer_t notificationBytes;
		cxa_fixedByteBuffer_init_subsetOfData(&notificationBytes, dataBytesIn, 2, CXA_FIXED_BYTE_BUFFER_LEN_ALL);
		
		cxa_serial_rpcProtocolManager_notificationListener_entry_t *nl = (cxa_serial_rpcProtocolManager_notificationListener_entry_t*)cxa_map_get(&srpmIn->notificationListeners, &opCode);
		if( nl == NULL )
		{
			cxa_logger_debug(&srpmIn->logger, "received unexpected notification opCode[0x%02X]", opCode);
			
			// notify our listener
			if( srpmIn->unknownNotificationListener.cb != NULL ) srpmIn->unknownNotificationListener.cb(&notificationBytes, srpmIn->unknownNotificationListener.userVar);
		}else if( nl->cb != NULL ) nl->cb(&notificationBytes, nl->userVar);
	}
	else
	{
		cxa_logger_debug(&srpmIn->logger, "unknown message type: 0x%02X", msgType);
	}
}