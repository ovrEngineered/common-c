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
#include "cxa_mqtt_message_publish.h"


// ******** includes ********
#include <cxa_assert.h>

#define CXA_LOG_LEVEL				CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
bool cxa_mqtt_message_publish_init(cxa_mqtt_message_t *const msgIn, bool dupIn, cxa_mqtt_qosLevel_t qosIn, bool retainIn, char *const topicNameIn, uint16_t packedIdIn, void *const payloadIn, size_t payloadSize_bytesIn)
{
	cxa_assert(msgIn);

	return false;
}


bool cxa_mqtt_message_publish_getTopicName(cxa_mqtt_message_t *const msgIn, char** topicNameOut, size_t *const topicNameLen_bytesOut)
{
	cxa_assert(msgIn);

	if( !msgIn->areFieldsConfigured || (cxa_mqtt_message_getType(msgIn) != CXA_MQTT_MSGTYPE_PUBLISH) ) return false;

	return cxa_linkedField_get_lengthPrefixedCString_uint16BE_inPlace(&msgIn->fields_publish.field_topicName, 0, topicNameOut, topicNameLen_bytesOut);
}


bool cxa_mqtt_message_publish_getPayload(cxa_mqtt_message_t *const msgIn, void** payloadOut, size_t *const payloadSize_bytesOut)
{
	cxa_assert(msgIn);

	if( !msgIn->areFieldsConfigured || (cxa_mqtt_message_getType(msgIn) != CXA_MQTT_MSGTYPE_PUBLISH) ) return false;

	// get a pointer to our data
	void* payloadPtr = cxa_linkedField_get_pointerToIndex(&msgIn->fields_publish.field_payload, 0);
	if( payloadPtr == NULL ) return false;

	if( payloadOut != NULL ) *payloadOut = payloadPtr;
	if( payloadSize_bytesOut != NULL ) *payloadSize_bytesOut = cxa_linkedField_getSize_bytes(&msgIn->fields_publish.field_payload);

	return true;
}


bool cxa_mqtt_message_publish_validateReceivedBytes(cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(msgIn);

	// first up is the topic name
	uint16_t numBytesInTopicName;
	if( !cxa_fixedByteBuffer_get_lengthPrefixedCString_uint16BE(msgIn->buffer, cxa_linkedField_getStartIndexOfNextField(&msgIn->field_remainingLength), NULL, &numBytesInTopicName, NULL) ||
			!cxa_linkedField_initChild(&msgIn->fields_publish.field_topicName, &msgIn->field_remainingLength, numBytesInTopicName+2) ) return false;

	// packet id
	if( !cxa_linkedField_initChild_fixedLen(&msgIn->fields_publish.field_packetId, &msgIn->fields_publish.field_topicName, 2) ) return false;

	// payload
	uint16_t numBytesInPayload = cxa_fixedByteBuffer_getSize_bytes(msgIn->buffer) - cxa_linkedField_getStartIndexOfNextField(&msgIn->fields_publish.field_packetId);
	if( !cxa_linkedField_initChild(&msgIn->fields_publish.field_payload, &msgIn->fields_publish.field_packetId, numBytesInPayload) ) return false;

	return true;
}


// ******** local function implementations ********
