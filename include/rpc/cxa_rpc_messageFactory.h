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
#ifndef CXA_RPC_MESSAGE_FACTORY_H_
#define CXA_RPC_MESSAGE_FACTORY_H_


// ******** includes ********
#include <cxa_rpc_message.h>
#include <cxa_config.h>


// ******** global macro definitions ********
#ifndef CXA_RPC_MSGFACTORY_POOL_NUM_MSGS
	#define CXA_RPC_MSGFACTORY_POOL_NUM_MSGS				2
#endif


// ******** global type definitions *********


// ******** global function prototypes ********
void cxa_rpc_messageFactory_init(void);

size_t cxa_rpc_messageFactory_getNumFreeMessages(void);
cxa_rpc_message_t* cxa_rpc_messageFactory_getFreeMessage_empty(void);

void cxa_rpc_messageFactory_incrementMessageRefCount(cxa_rpc_message_t *const msgIn);
void cxa_rpc_messageFactory_decrementMessageRefCount(cxa_rpc_message_t *const msgIn);
uint8_t cxa_rpc_messageFactory_getReferenceCountForMessage(cxa_rpc_message_t *const msgIn);


#endif // CXA_RPC_NODE_H_
