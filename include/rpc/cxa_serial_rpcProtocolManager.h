/**
 * @file
 *
 * @note This object should work across all architecture-specific implementations
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * @endcode
 *
 *
 * @copyright 2013-2014 opencxa.org
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
#ifndef CXA_SERIAL_RPCPROTOCOLMANAGER_H_
#define CXA_SERIAL_RPCPROTOCOLMANAGER_H_


// ******** includes ********
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <cxa_array.h>
#include <cxa_logger_header.h>
#include <cxa_map.h>
#include <cxa_serial_protocolParser.h>
#include <cxa_timeBase.h>


// ******** global macro definitions ********
#ifndef CXA_SERIAL_RPCPROTOCOLMANAGER_MAX_NUM_INFLIGHTREQS
	#define CXA_SERIAL_RPCPROTOCOLMANAGER_MAX_NUM_INFLIGHTREQS				1
#endif
#ifndef CXA_SERIAL_RPCPROTOCOLMANAGER_MAX_NUM_REQLISTENERS
	#define CXA_SERIAL_RPCPROTOCOLMANAGER_MAX_NUM_REQLISTENERS				1
#endif
#ifndef CXA_SERIAL_RPCPROTOCOLMANAGER_MAX_NUM_NOTILISTENERS
	#define CXA_SERIAL_RPCPROTOCOLMANAGER_MAX_NUM_NOTILISTENERS				1
#endif


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_serial_rpcProtocolManager_t object
 */
typedef struct cxa_serial_rpcProtocolManager cxa_serial_rpcProtocolManager_t;

/**
 * @public
 * @brief Callback called when/if a request is received.
 *
 * @param[in] bufferIn a buffer containing the message data
 *		(no header or footer information)
 * @param[out] responseBytesOut a buffer into which response
 *		data should be put
 * @param[in] userVarIn pointer to the user-supplied variable passed to
 *		::cxa_serial_rpcProtocolManager_addRequestListener
 *
 * @return true if we should send a response (using the data in the responseBytes parameter)
 */
typedef bool (*cxa_serial_rpcProtocolManager_cb_requestReceived_t)(cxa_fixedByteBuffer_t *const dataBytesIn, cxa_fixedByteBuffer_t *const responseBytesOut, void *const userVarIn);

/**
 * @public
 * @brief Callback called when/if a notification is received
 *
 * @param[in] bufferIn a buffer containing the message data
 *		(no header or footer information)
 * @param[in] userVarIn pointer to the user-supplied variable passed to
 *		::cxa_serial_rpcProtocolManager_addNotificationListener
 */
typedef void (*cxa_serial_rpcProtocolManager_cb_notificationReceived_t)(cxa_fixedByteBuffer_t *const dataBytesIn, void *const userVarIn);


/**
 * @private
 */
typedef struct
{
	cxa_serial_rpcProtocolManager_cb_requestReceived_t cb;
	
	void *userVar;
}cxa_serial_rpcProtocolManager_requestListener_entry_t;


/**
 * @private
 */
typedef struct
{
	cxa_serial_rpcProtocolManager_cb_notificationReceived_t cb;
	
	void *userVar;
}cxa_serial_rpcProtocolManager_notificationListener_entry_t;


/**
 * @private
 */
typedef struct
{
	volatile bool responseReceived;
	
	cxa_fixedByteBuffer_t *responseBytes;
}cxa_serial_rpcProtocolManager_inflightRequest_t;


/**
 * @private
 */
typedef struct
{
	cxa_fixedByteBuffer_t dataBytes;
	cxa_fixedByteBuffer_t *parentBuffer;
}cxa_serial_rpcProtocolManager_usedMsgMap_entry_t;


/**
 * @private
 */
struct cxa_serial_rpcProtocolManager
{
	cxa_logger_t logger;
	cxa_serial_protocolParser_t protoParser;
	
	uint16_t nextId;
	cxa_timeBase_t *timeBase;
	
	cxa_map_t inflightRequests;
	uint8_t inflightRequests_raw[CXA_MAP_CALC_BUFFER_SIZE(uint16_t, cxa_serial_rpcProtocolManager_inflightRequest_t, CXA_SERIAL_RPCPROTOCOLMANAGER_MAX_NUM_INFLIGHTREQS)];
	
	cxa_map_t requestListeners;
	uint8_t requestListeners_raw[CXA_MAP_CALC_BUFFER_SIZE(uint8_t, cxa_serial_rpcProtocolManager_requestListener_entry_t, CXA_SERIAL_RPCPROTOCOLMANAGER_MAX_NUM_REQLISTENERS)];
	
	cxa_map_t notificationListeners;
	uint8_t notificationListeners_raw[CXA_MAP_CALC_BUFFER_SIZE(uint8_t, cxa_serial_rpcProtocolManager_notificationListener_entry_t, CXA_SERIAL_RPCPROTOCOLMANAGER_MAX_NUM_NOTILISTENERS)];
	
	cxa_serial_rpcProtocolManager_requestListener_entry_t unknownRequestListener;
	cxa_serial_rpcProtocolManager_notificationListener_entry_t unknownNotificationListener;
	
	cxa_array_t usedMessageMap;
	cxa_serial_rpcProtocolManager_usedMsgMap_entry_t usedMessageMap_raw[CXA_SERIAL_PROTOCOLPARSER_MSG_POOL_NUM_MSGS];
};


// ******** global function prototypes ********
void cxa_serial_rpcProtocolManager_init(cxa_serial_rpcProtocolManager_t *const srpmIn, cxa_timeBase_t *const tbIn);

void cxa_serial_rpcProtocolManager_start(cxa_serial_rpcProtocolManager_t *const srpmIn, FILE *const fdIn);
void cxa_serial_rpcProtocolManager_stop(cxa_serial_rpcProtocolManager_t *const srpmIn);

cxa_fixedByteBuffer_t* cxa_serial_rpcProtocolManager_sendRequest_getFreeBuffer(cxa_serial_rpcProtocolManager_t *const srpmIn, uint8_t opCodeIn);
cxa_fixedByteBuffer_t* cxa_serial_rpcProtocolManager_sendRequest_sync(cxa_serial_rpcProtocolManager_t *const srpmIn, cxa_fixedByteBuffer_t *const dataBytesIn);
void cxa_serial_rpcProtocolManager_sendRequest_returnResponseBuffer(cxa_serial_rpcProtocolManager_t *const srpmIn, cxa_fixedByteBuffer_t *const dataBytesIn);

cxa_fixedByteBuffer_t* cxa_serial_rpcProtocolManager_sendNotification_getBuffer(cxa_serial_rpcProtocolManager_t *const srpmIn, uint8_t opCodeIn);
void cxa_serial_rpcProtocolManager_sendNotification(cxa_serial_rpcProtocolManager_t *const srpmIn, cxa_fixedByteBuffer_t *const dataBytesIn);

void cxa_serial_rpcProtocolManager_addRequestListener(cxa_serial_rpcProtocolManager_t *const srpmIn, uint8_t opCodeIn, cxa_serial_rpcProtocolManager_cb_requestReceived_t cbIn, void *const userVarIn);
void cxa_serial_rpcProtocolManager_addNotificationListener(cxa_serial_rpcProtocolManager_t *const srpmIn, uint8_t opCodeIn, cxa_serial_rpcProtocolManager_cb_notificationReceived_t cbIn, void *const userVarIn);

void cxa_serial_rpcProtocolManager_setUnknownRequestListener(cxa_serial_rpcProtocolManager_t *const srpmIn, cxa_serial_rpcProtocolManager_cb_requestReceived_t cbIn, void *const userVarIn);
void cxa_serial_rpcProtocolManager_setUnknownNotificationListener(cxa_serial_rpcProtocolManager_t *const srpmIn, cxa_serial_rpcProtocolManager_cb_notificationReceived_t cbIn, void *const userVarIn);

void cxa_serial_rpcProtocolManager_update(cxa_serial_rpcProtocolManager_t *const srpmIn);


#endif // CXA_SERIAL_RPCPROTOCOLMANAGER_H_
