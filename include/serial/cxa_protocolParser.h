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
#ifndef CXA_RPC_PROTOCOLPARSER_H_
#define CXA_RPC_PROTOCOLPARSER_H_


// ******** includes ********
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <cxa_array.h>
#include <cxa_ioStream.h>
#include <cxa_logger_header.h>
#include <cxa_stateMachine.h>
#include <cxa_rpc_message.h>


// ******** global macro definitions ********
#ifndef CXA_RPC_PROTOCOLPARSER_MAXNUM_PROTOLISTENERS
	#define CXA_RPC_PROTOCOLPARSER_MAXNUM_PROTOLISTENERS		1
#endif
#ifndef CXA_RPC_PROTOCOLPARSER_MAXNUM_MSGLISTENERS
	#define CXA_RPC_PROTOCOLPARSER_MAXNUM_MSGLISTENERS			1
#endif


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_rpc_protocolParser_t object
 */
typedef struct cxa_rpc_protocolParser cxa_rpc_protocolParser_t;


/**
 * @public
 * @brief Callback called when/if a message is received with an unsupported version number.
 * 
 * @param[in] bufferIn a buffer containing the entire message
 * @param[in] userVarIn pointer to the user-supplied variable passed to
 *		::cxa_rpc_protocolParser_addProtocolListener
 */
typedef void (*cxa_rpc_protocolParser_cb_invalidVersionNumber_t)(cxa_fixedByteBuffer_t *const bufferIn, void *const userVarIn);


/**
 * @public
 * @brief Callback called when/if an error occurs reading/writing to/from the serial device
 *
 * @param[in] userVarIn pointer to the user-supplied variable passed to
 *		::cxa_rpc_protocolParser_addProtocolListener
 */
typedef void (*cxa_rpc_protocolParser_cb_ioExceptionOccurred_t)(void *const userVarIn);


typedef void (*cxa_rpc_protocolParser_cb_messageReceived_t)(cxa_rpc_message_t *const msgIn, void *const userVarIn);


/**
 * @private
 */
typedef struct
{
	cxa_rpc_protocolParser_cb_invalidVersionNumber_t cb_invalidVer;
	cxa_rpc_protocolParser_cb_ioExceptionOccurred_t cb_exception;
	
	void *userVar;
}cxa_rpc_protocolParser_protoListener_entry_t;


/**
 * @private
 */
typedef struct
{
	cxa_rpc_protocolParser_cb_messageReceived_t cb;
	
	void *userVar;
}cxa_rpc_protocolParser_messageListener_entry_t;



/**
 * @private
 */
struct cxa_rpc_protocolParser
{
	cxa_logger_t logger;

	cxa_array_t protocolListeners;
	cxa_rpc_protocolParser_protoListener_entry_t protocolListeners_raw[CXA_RPC_PROTOCOLPARSER_MAXNUM_PROTOLISTENERS];

	cxa_array_t messageListeners;
	cxa_rpc_protocolParser_messageListener_entry_t messageListeners_raw[CXA_RPC_PROTOCOLPARSER_MAXNUM_MSGLISTENERS];
	
	uint8_t userProtoVersion;
	cxa_stateMachine_t stateMachine;
	cxa_ioStream_t *ioStream;
	
	cxa_rpc_message_t *currRxMsg;
};


// ******** global function prototypes ********
void cxa_rpc_protocolParser_init(cxa_rpc_protocolParser_t *const rppIn, uint8_t userProtoVersionIn, cxa_ioStream_t *ioStreamIn);
void cxa_rpc_protocolParser_deinit(cxa_rpc_protocolParser_t *const rppIn);

bool cxa_rpc_protocolParser_writeMessage(cxa_rpc_protocolParser_t *const rppIn, cxa_rpc_message_t *const msgToWriteIn);

void cxa_rpc_protocolParser_addProtocolListener(cxa_rpc_protocolParser_t *const rppIn,
		cxa_rpc_protocolParser_cb_invalidVersionNumber_t cb_invalidVerIn,
		cxa_rpc_protocolParser_cb_ioExceptionOccurred_t cb_exceptionIn,
		void *const userVarIn);

void cxa_rpc_protocolParser_addMessageListener(cxa_rpc_protocolParser_t *const rppIn,
		cxa_rpc_protocolParser_cb_messageReceived_t cb_msgRxIn,
		void *const userVarIn);
	
void cxa_rpc_protocolParser_update(cxa_rpc_protocolParser_t *const rppIn);


#endif // cxa_rpc_PROTOCOLPARSER_H_
