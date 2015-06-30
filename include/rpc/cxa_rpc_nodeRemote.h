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
#ifndef CXA_RPC_NODEREMOTE_H_
#define CXA_RPC_NODEREMOTE_H_


// ******** includes ********
#include <stdbool.h>
#include <cxa_array.h>
#include <cxa_logger_header.h>
#include <cxa_timeBase.h>
#include <cxa_rpc_messageHandler.h>
#include <cxa_rpc_protocolParser.h>
#include <cxa_timeDiff.h>

#include <cxa_config.h>


// ******** global macro definitions ********
#ifndef CXA_RPC_NODEREMOTE_MAX_NUM_LINK_LISTENERS
	#define CXA_RPC_NODEREMOTE_MAX_NUM_LINK_LISTENERS		2
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef void (*cxa_rpc_nodeRemote_cb_linkEstablished_t)(cxa_rpc_nodeRemote_t *const nrIn, void* userVarIn);


/**
 * @private
 */
typedef struct
{
	cxa_rpc_nodeRemote_cb_linkEstablished_t cb_linkEstablished;
	void *userVar;
}cxa_rpc_nodeRemote_linkListener_t;


/**
 * @private
 */
struct cxa_rpc_nodeRemote
{
	cxa_rpc_messageHandler_t super;

	cxa_rpc_protocolParser_t protocolParser;
	cxa_rpc_node_t *downstreamSubNode;

	cxa_timeDiff_t td_askForName;

	cxa_array_t linkListeners;
	cxa_rpc_nodeRemote_linkListener_t linkListeners_raw[CXA_RPC_NODEREMOTE_MAX_NUM_LINK_LISTENERS];

	cxa_logger_t logger;
};


// ******** global function prototypes ********
void cxa_rpc_nodeRemote_init_upstream(cxa_rpc_nodeRemote_t *const nrIn, cxa_ioStream_t *const ioStreamIn, cxa_timeBase_t *const timeBaseIn);
bool cxa_rpc_nodeRemote_init_downstream(cxa_rpc_nodeRemote_t *const nrIn, cxa_ioStream_t *const ioStreamIn, cxa_rpc_node_t *const subNodeIn);

bool cxa_rpc_nodeRemote_addLinkListener(cxa_rpc_nodeRemote_t *const nrIn, cxa_rpc_nodeRemote_cb_linkEstablished_t cb_linkEstablishedIn, void *const userVarIn);

void cxa_rpc_nodeRemote_update(cxa_rpc_nodeRemote_t *const nrIn);

#endif // CXA_RPC_NODEREMOTE_H_
