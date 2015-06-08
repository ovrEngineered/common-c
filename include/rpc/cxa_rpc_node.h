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
#ifndef CXA_RPC_NODE_H_
#define CXA_RPC_NODE_H_


// ******** includes ********
#include <stdbool.h>
#include <cxa_array.h>


// ******** global macro definitions ********
#ifndef CXA_RPC_NODE_MAX_NUM_SUBNODES
	#define CXA_RPC_NODE_MAX_NUM_SUBNODES				5
#endif


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_rpc_node_t object
 */
typedef struct cxa_rpc_node cxa_rpc_node_t;


typedef bool (*cxa_rpc_node_methodCb_t)(cxa_rpc_node_t *const nodeIn, )


/**
 * @private
 */
struct cxa_rpc_node
{
	cxa_array_t subnodes;
	cxa_rpc_node_t subnodes_raw[CXA_RPC_NODE_MAX_NUM_SUBNODES];
};


// ******** global function prototypes ********
void cxa_rpc_node_init(cxa_rpc_node_t *const nodeIn, char *const nameIn);

void cxa_rpc_node_addSubNode(cxa_rpc_node_t *const nodeIn, cxa_rpc_node_t *const subNodeIn);
void cxa_rpc_node_addMethod(cxa_rpc_node_t *const nodeIn, char *const nameIn, )


#endif // CXA_RPC_NODE_H_
