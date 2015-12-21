/**
 * @file
 * @copyright 2015 opencxa.org
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
#ifndef CXA_MQTT_RPC_NODE_BRIDGE_MULTI_H_
#define CXA_MQTT_RPC_NODE_BRIDGE_MULTI_H_


// ******** includes ********
#include <cxa_mqtt_rpc_node_bridge.h>
#include <cxa_array.h>
#include <cxa_ioStream.h>
#include <cxa_logger_header.h>
#include <cxa_mqtt_rpc_node.h>
#include <cxa_protocolParser_mqtt.h>
#include <cxa_config.h>


// ******** global macro definitions ********
#ifndef CXA_MQTT_RPC_NODE_BRIDGE_MAXNUM_REMOTE_NODES
	#define CXA_MQTT_RPC_NODE_BRIDGE_MAXNUM_REMOTE_NODES			4
#endif


// ******** global type definitions *********
typedef struct cxa_mqtt_rpc_node_bridge_multi cxa_mqtt_rpc_node_bridge_multi_t;

typedef cxa_mqtt_rpc_node_bridge_authorization_t (*cxa_mqtt_rpc_node_bridge_multi_cb_authenticateClient_t)(char *const clientIdIn, size_t clientIdLen_bytes,
																											char *const usernameIn, size_t usernameLen_bytesIn,
																											uint8_t *const passwordIn, size_t passwordLen_bytesIn,
																											char* provisionedNameOut, size_t maxProvisionedNameLen_bytesIn,
																											void *userVarIn);

/**
 * @private
 */
typedef struct
{
	char clientId[CXA_MQTT_RPC_NODE_BRIDGE_CLIENTID_MAXLEN_BYTES];
	char mappedName[CXA_MQTT_RPC_NODE_BRIDGE_MAPPEDNAME_MAXLEN_BYTES];
}cxa_mqtt_rpc_node_bridge_multi_remoteNodeEntry_t;


/**
 * @private
 */
struct cxa_mqtt_rpc_node_bridge_multi
{
	cxa_mqtt_rpc_node_bridge_t super;

	cxa_array_t remoteNodes;
	cxa_mqtt_rpc_node_bridge_multi_remoteNodeEntry_t remoteNodes_raw[CXA_MQTT_RPC_NODE_BRIDGE_MAXNUM_REMOTE_NODES];

	cxa_mqtt_rpc_node_bridge_multi_cb_authenticateClient_t cb_localAuth;
	void* localAuthUserVar;
};


// ******** global function prototypes ********
void cxa_mqtt_rpc_node_bridge_multi_init(cxa_mqtt_rpc_node_bridge_multi_t *const nodeIn, cxa_mqtt_rpc_node_t *const parentNodeIn,
										 cxa_protocolParser_mqtt_t *const mppIn,
										 const char *nameFmtIn, ...);


void cxa_mqtt_rpc_node_bridge_multi_setAuthCb(cxa_mqtt_rpc_node_bridge_multi_t *const nodeIn, cxa_mqtt_rpc_node_bridge_multi_cb_authenticateClient_t authCbIn, void *const userVarIn);

void cxa_mqtt_rpc_node_bridge_multi_clearRemoteNodes(cxa_mqtt_rpc_node_bridge_multi_t *const nodeIn);

#endif // CXA_MQTT_RPC_NODEBRIDGE_MULTI_H_
