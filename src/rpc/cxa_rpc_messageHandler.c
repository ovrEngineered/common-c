/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_rpc_messageHandler.h"


// ******** includes ********
#include <stdio.h>
#include <string.h>
#include <cxa_assert.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_rpc_messageHandler_init(cxa_rpc_messageHandler_t *const handlerIn, cxa_rpc_messageHandler_cb_handleUpstream_t cb_upstreamIn, cxa_rpc_messageHandler_cb_handleDownstream_t cb_downstreamIn)
{
	cxa_assert(handlerIn);
	cxa_assert(cb_upstreamIn);
	cxa_assert(cb_downstreamIn);

	// save our internal state
	handlerIn->cb_upstream = cb_upstreamIn;
	handlerIn->cb_downstream = cb_downstreamIn;
	handlerIn->parent = NULL;
}


// ******** local function implementations ********
