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
#ifndef CXA_MQTT_RPC_NODE_BRIDGE_SINGLE_H_
#define CXA_MQTT_RPC_NODE_BRIDGE_SINGLE_H_


// ******** includes ********
#include <cxa_mqtt_rpc_node_bridge.h>
#include <cxa_array.h>
#include <cxa_ioStream.h>
#include <cxa_logger_header.h>
#include <cxa_mqtt_rpc_node.h>
#include <cxa_protocolParser_mqtt.h>
#include <cxa_config.h>


// ******** global macro definitions ********
#ifndef CXA_MQTT_RPC_NODE_BRIDGE_CLIENTID_MAXLEN_BYTES
	#define CXA_MQTT_RPC_NODE_BRIDGE_CLIENTID_MAXLEN_BYTES			17
#endif


// ******** global type definitions *********
typedef struct cxa_mqtt_rpc_node_bridge_single cxa_mqtt_rpc_node_bridge_single_t;


/**
 * @private
 */
struct cxa_mqtt_rpc_node_bridge_single
{
	cxa_mqtt_rpc_node_bridge_t super;

	bool hasClientAuthed;
	char clientId[CXA_MQTT_RPC_NODE_BRIDGE_CLIENTID_MAXLEN_BYTES+1];
};


// ******** global function prototypes ********
void cxa_mqtt_rpc_node_bridge_single_init(cxa_mqtt_rpc_node_bridge_single_t *const nodeIn, cxa_mqtt_rpc_node_t *const parentNodeIn,
										 cxa_ioStream_t *const iosIn, cxa_timeBase_t *const timeBaseIn, const char *nameFmtIn, ...);

#endif // CXA_MQTT_RPC_NODEBRIDGE_SINGLE_H_
