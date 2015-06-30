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
#include "cxa_rpc_messageHandler.h"


/**
 * @author Christopher Armenio
 */


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
	handlerIn->name[0] = 0;
	handlerIn->parent = NULL;
	handlerIn->hasName = false;
}


const char *const cxa_rpc_messageHandler_getName(cxa_rpc_messageHandler_t *const handlerIn)
{
	cxa_assert(handlerIn);

	return (handlerIn->hasName) ? handlerIn->name : NULL;
}


void cxa_rpc_messageHandler_setName(cxa_rpc_messageHandler_t *const handlerIn, const char *nameFmtIn, va_list varArgsIn)
{
	cxa_assert(handlerIn);
	cxa_assert(nameFmtIn);

	// save our name (and make sure it's null-terminated)
	vsnprintf(handlerIn->name, CXA_RPC_NODE_MAX_NAME_LEN_BYTES, nameFmtIn, varArgsIn);
	handlerIn->name[CXA_RPC_NODE_MAX_NAME_LEN_BYTES-1] = 0;

	handlerIn->hasName = true;
}


// ******** local function implementations ********
