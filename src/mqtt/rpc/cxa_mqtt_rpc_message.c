/**
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
#include "cxa_mqtt_rpc_message.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_mqtt_rpc_node.h>
#include <cxa_mqtt_message_publish.h>
#include <cxa_stringUtils.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
bool cxa_mqtt_rpc_message_isActionableResponse(cxa_mqtt_message_t *const msgIn, char** methodNameOut, size_t* methodNameLen_bytesOut, char** idOut, size_t* idLen_bytesOut)
{
	cxa_assert(msgIn);

	// make sure it is a publish
	if( cxa_mqtt_message_getType(msgIn) != CXA_MQTT_MSGTYPE_PUBLISH ) return false;

	// we need to get the topic
	char* topic;
	uint16_t topicLen_bytes;
	if( !cxa_mqtt_message_publish_getTopicName(msgIn, &topic, &topicLen_bytes) ) return false;

	ssize_t methodIndex = cxa_stringUtils_indexOfFirstOccurence_withLengths(topic, topicLen_bytes, CXA_MQTT_RPCNODE_RESP_PREFIX, strlen(CXA_MQTT_RPCNODE_RESP_PREFIX));
	if( methodIndex < 0 ) return false;
	methodIndex += 2;

	// if we made it here, it must be a request...parse out the method and id
	if( methodNameOut != NULL ) *methodNameOut = &topic[methodIndex];
	ssize_t separatorIndex = cxa_stringUtils_indexOfFirstOccurence_withLengths(&topic[methodIndex], (topicLen_bytes - methodIndex), "/", 1);
	if( separatorIndex < 0 ) return false;
	separatorIndex += methodIndex;

	size_t methodNameLen_bytes = (separatorIndex - methodIndex);
	if( methodNameLen_bytes >= CXA_MQTT_RPCNODE_MAXLEN_METHOD_BYTES ) return false;
	if( methodNameLen_bytesOut != NULL ) *methodNameLen_bytesOut = methodNameLen_bytes;

	// now the ID...should be 4 characters
	ssize_t idIndex = separatorIndex+1;
	if( idIndex+4 > topicLen_bytes ) return false;
	if( idOut != NULL ) *idOut = &topic[idIndex];

	size_t idLen_bytes = (topicLen_bytes - idIndex);
	if( idLen_bytes > 4 ) return false;
	if( idLen_bytesOut != NULL ) *idLen_bytesOut = idLen_bytes;

	return true;
}

// ******** local function implementations ********
