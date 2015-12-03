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
#ifndef CXA_MQTT_RPC_NODE_TEMP_H_
#define CXA_MQTT_RPC_NODE_TEMP_H_


// ******** includes ********
#include <cxa_mqtt_rpc_node.h>
#include <cxa_timeDiff.h>
#include <cxa_config.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct cxa_mqtt_rpc_node_temp cxa_mqtt_rpc_node_temp_t;


/**
 * @private
 */
struct cxa_mqtt_rpc_node_temp
{
	cxa_mqtt_rpc_node_t super;

	cxa_timeDiff_t td_sample;
	uint32_t updatePeriod_s;

	float lastReading_c;
	float upperBound_c;
	float lowerBound_c;
};


// ******** global function prototypes ********
void cxa_mqtt_rpc_node_temp_init_subnode(cxa_mqtt_rpc_node_temp_t *const nodeIn, cxa_mqtt_rpc_node_t *const parentNodeIn, char *const nameIn, cxa_timeBase_t *const timeBaseIn);
void cxa_mqtt_rpc_node_temp_update(cxa_mqtt_rpc_node_temp_t * const nodeIn);

#endif // CXA_MQTT_RPC_NODE_TEMP_H_
