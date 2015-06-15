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
#include "cxa_rpc_node.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdio.h>
#include <string.h>
#include <cxa_assert.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_DEBUG
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_rpc_node_init(cxa_rpc_node_t *const nodeIn, char *const nameIn)
{
	cxa_assert(nodeIn);
	cxa_assert(nameIn);

	// save our name (and make sure it's null-terminated)
	strncpy(nodeIn->name, nameIn, CXA_RPC_NODE_MAX_NAME_LEN_BYTES);
	nodeIn->name[CXA_RPC_NODE_MAX_NAME_LEN_BYTES-1] = 0;

	// setup our initial state
	nodeIn->parent = NULL;
	cxa_array_initStd(&nodeIn->subnodes, nodeIn->subnodes_raw);

	// setup our logger
	cxa_logger_vinit(&nodeIn->logger, "rpcNode_%s", nodeIn->name);
}


bool cxa_rpc_node_addSubNode(cxa_rpc_node_t *const nodeIn, cxa_rpc_node_t *const subNodeIn)
{
	cxa_assert(nodeIn);
	cxa_assert(subNodeIn);

	if( subNodeIn->parent != NULL )
	{
		cxa_logger_warn(&nodeIn->logger, "attempted subnode %p already has parent", subNodeIn);
		return false;
	}

	if( !cxa_array_append(&nodeIn->subnodes, (void*)&subNodeIn) ) return false;
	subNodeIn->parent = nodeIn;

	return true;
}


// ******** local function implementations ********
