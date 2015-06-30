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
#ifndef CXA_RPC_MESSAGE_HANDLER_H_
#define CXA_RPC_MESSAGE_HANDLER_H_


// ******** includes ********
#include <cxa_rpc_message.h>
#include <cxa_config.h>
#include <stdarg.h>


// ******** global macro definitions ********
#ifndef CXA_RPC_NODE_MAX_NAME_LEN_BYTES
	#define CXA_RPC_NODE_MAX_NAME_LEN_BYTES				10
#endif


#define cxa_rpc_messageHandler_handleUpstream(handlerIn, msgIn)			if( (handlerIn)->cb_upstream != NULL ) (handlerIn)->cb_upstream((handlerIn), (msgIn))
#define cxa_rpc_messageHandler_handleDownstream(handlerIn, msgIn)		if( (handlerIn)->cb_downstream != NULL ) (handlerIn)->cb_downstream((handlerIn), (msgIn))


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_rpc_messageHandler_t object
 */
typedef struct cxa_rpc_messageHandler cxa_rpc_messageHandler_t;


/**
 * @public
 * @brief "Forward" declaration of the cxa_rpc_node_t object
 */
typedef struct cxa_rpc_node cxa_rpc_node_t;


/**
 * @public
 * @brief "Forward" declaration of the cxa_rpc_nodeRemote_t object
 */
typedef struct cxa_rpc_nodeRemote cxa_rpc_nodeRemote_t;


/**
 * @public
 */
typedef void (*cxa_rpc_messageHandler_cb_handleUpstream_t)(cxa_rpc_messageHandler_t *const handlerIn, cxa_rpc_message_t *const msgIn);


/**
 * @public
 */
typedef void (*cxa_rpc_messageHandler_cb_handleDownstream_t)(cxa_rpc_messageHandler_t *const handlerIn, cxa_rpc_message_t *const msgIn);


/**
 * @private
 */
struct cxa_rpc_messageHandler
{
	bool hasName;
	char name[CXA_RPC_NODE_MAX_NAME_LEN_BYTES+1];

	cxa_rpc_messageHandler_cb_handleUpstream_t cb_upstream;
	cxa_rpc_messageHandler_cb_handleDownstream_t cb_downstream;

	cxa_rpc_messageHandler_t* parent;
};


// ******** global function prototypes ********
void cxa_rpc_messageHandler_init(cxa_rpc_messageHandler_t *const handlerIn, cxa_rpc_messageHandler_cb_handleUpstream_t cb_upstreamIn, cxa_rpc_messageHandler_cb_handleDownstream_t cb_downstreamIn);

const char *const cxa_rpc_messageHandler_getName(cxa_rpc_messageHandler_t *const handlerIn);
void cxa_rpc_messageHandler_setName(cxa_rpc_messageHandler_t *const handlerIn, const char *nameFmtIn, va_list varArgsIn);


#endif // CXA_RPC_MESSAGE_HANDLER_H_
