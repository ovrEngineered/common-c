/**
 * Copyright 2015 opencxa.org
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
 *
 * @author Christopher Armenio
 */
#include "cxa_mqtt_rpc_node_bridge.h"


// ******** includes ********
#include <cxa_assert.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_mqtt_rpc_node_bridge_init(cxa_mqtt_rpc_node_bridge_t *const nodeIn, cxa_mqtt_rpc_node_t *const parentNodeIn, char *const nameIn,
								   cxa_ioStream_t *const iosIn, cxa_timeBase_t *const timeBaseIn)
{
	cxa_assert(nodeIn);
	cxa_assert(iosIn);
	cxa_assert(timeBaseIn);

	// set some defaults
	nodeIn->cb_auth = NULL;
	nodeIn->userVar_auth = NULL;
}


void cxa_mqtt_rpc_node_bridge_setAuthenticationCb(cxa_mqtt_rpc_node_bridge_t *const nodeIn, cxa_mqtt_rpc_node_bridge_cb_authenticateClient_t cb_authIn, void* userVarIn)
{
	cxa_assert(nodeIn);

	nodeIn->cb_auth = cb_authIn;
	nodeIn->userVar_auth = userVarIn;
}


void cxa_mqtt_rpc_node_bridge_update(cxa_mqtt_rpc_node_bridge_t *const nodeIn)
{
	cxa_assert(nodeIn);
}


// ******** local function implementations ********
