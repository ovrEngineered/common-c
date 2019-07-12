/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_RPC_MESSAGE_FACTORY_H_
#define CXA_RPC_MESSAGE_FACTORY_H_


// ******** includes ********
#include <stdio.h>
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

cxa_rpc_message_t* cxa_rpc_messageFactory_getMessage_byBuffer(cxa_fixedByteBuffer_t *const fbbIn);

void cxa_rpc_messageFactory_incrementMessageRefCount(cxa_rpc_message_t *const msgIn);
void cxa_rpc_messageFactory_decrementMessageRefCount(cxa_rpc_message_t *const msgIn);
uint8_t cxa_rpc_messageFactory_getReferenceCountForMessage(cxa_rpc_message_t *const msgIn);

#endif // CXA_RPC_NODE_H_
