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
#ifndef CXA_MQTT_RPC_NODE_ROOT_H_
#define CXA_MQTT_RPC_NODE_ROOT_H_


// ******** includes ********
#include <cxa_mqtt_client.h>
#include <cxa_mqtt_rpc_node.h>
#include <cxa_timeDiff.h>
#include <cxa_config.h>


// ******** global macro definitions ********
#ifndef CXA_MQTT_RPC_NODE_ROOT_MAXLEN_NAME_AND_ROOTPREFIX_BYTES
	#define CXA_MQTT_RPC_NODE_ROOT_MAXLEN_NAME_AND_ROOTPREFIX_BYTES			24
#endif



// ******** global type definitions *********
typedef struct cxa_mqtt_rpc_node_root cxa_mqtt_rpc_node_root_t;


/**
 * @private
 */
struct cxa_mqtt_rpc_node_root
{
	cxa_mqtt_rpc_node_t super;

	cxa_mqtt_client_t* mqttClient;

	char nameAndRootPrefix[CXA_MQTT_RPC_NODE_ROOT_MAXLEN_NAME_AND_ROOTPREFIX_BYTES+1];
};


// ******** global function prototypes ********
void cxa_mqtt_rpc_node_root_init(cxa_mqtt_rpc_node_root_t *const nodeIn, char *const nameIn, char *const rootPrefixIn, cxa_mqtt_client_t* const clientIn);
cxa_mqtt_client_t* cxa_mqtt_rpc_node_root_getMqttClient(cxa_mqtt_rpc_node_root_t *const nodeIn);

#endif // CXA_MQTT_RPC_NODE_ROOT_H_
