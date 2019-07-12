/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_RPC_MESSAGE_HANDLER_H_
#define CXA_RPC_MESSAGE_HANDLER_H_


// ******** includes ********
#include <cxa_rpc_message.h>
#include <cxa_config.h>
#include <stdarg.h>
#include <cxa_logger_header.h>


// ******** global macro definitions ********
#define cxa_rpc_messageHandler_handleUpstream(handlerIn, msgIn)			if( (handlerIn)->cb_upstream != NULL ) (handlerIn)->cb_upstream((handlerIn), (msgIn))
#define cxa_rpc_messageHandler_handleDownstream(handlerIn, msgIn)		(((handlerIn)->cb_downstream != NULL) ? (handlerIn)->cb_downstream((handlerIn), (msgIn)) : false)


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
 *
 * @return true if message has been handled, false if not
 */
typedef bool (*cxa_rpc_messageHandler_cb_handleDownstream_t)(cxa_rpc_messageHandler_t *const handlerIn, cxa_rpc_message_t *const msgIn);


/**
 * @private
 */
struct cxa_rpc_messageHandler
{
	cxa_rpc_messageHandler_cb_handleUpstream_t cb_upstream;
	cxa_rpc_messageHandler_cb_handleDownstream_t cb_downstream;

	cxa_rpc_messageHandler_t* parent;
	cxa_logger_t logger;
};


// ******** global function prototypes ********
void cxa_rpc_messageHandler_init(cxa_rpc_messageHandler_t *const handlerIn, cxa_rpc_messageHandler_cb_handleUpstream_t cb_upstreamIn, cxa_rpc_messageHandler_cb_handleDownstream_t cb_downstreamIn);


#endif // CXA_RPC_MESSAGE_HANDLER_H_
