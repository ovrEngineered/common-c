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
bool cxa_mqtt_message_publish_init(cxa_mqtt_message_t *const msgIn, bool dupIn, cxa_mqtt_qosLevel_t qosIn, bool retainIn, char *const topicNameIn, uint16_t packedIdIn, void *const payloadIn, uint16_t payloadSize_bytesIn)
{
	cxa_assert(msgIn);
	cxa_assert(topicNameIn);
	if( payloadSize_bytesIn > 0 ) cxa_assert(payloadIn);

	// fixed header 1
	if( !cxa_linkedField_initRoot_fixedLen(&msgIn->field_packetTypeAndFlags, msgIn->buffer, 0, 1) ||
			!cxa_linkedField_append_uint8(&msgIn->field_packetTypeAndFlags, ((CXA_MQTT_MSGTYPE_PUBLISH << 4) | (dupIn << 3) | (qosIn << 1) | retainIn)) ) return false;

	// remaining length
	if( !cxa_linkedField_initChild(&msgIn->field_remainingLength, &msgIn->field_packetTypeAndFlags, 0) ) return false;

	// topic name
	if( !cxa_linkedField_initChild(&msgIn->fields_publish.field_topicName, &msgIn->field_remainingLength, 0) ||
			!cxa_linkedField_append_lengthPrefixedCString_uint16BE(&msgIn->fields_publish.field_topicName, topicNameIn, false) ) return false;
	cxa_linkedField_t* prevField = &msgIn->fields_publish.field_topicName;

	// packet identifier (if higher-level QOS)
	if( qosIn != CXA_MQTT_QOS_ATMOST_ONCE )
	{
		if( !cxa_linkedField_initRoot_fixedLen(&msgIn->fields_publish.field_packetId, msgIn->buffer, 0, 2) ||
					!cxa_linkedField_append_uint16BE(&msgIn->fields_publish.field_packetId, packedIdIn) ) return false;
		prevField = &msgIn->fields_publish.field_packetId;
	}

	// payload
	if( !cxa_linkedField_initChild(&msgIn->fields_publish.field_payload, prevField, 0) ) return false;
	if( payloadIn != NULL ) if( !cxa_linkedField_append_lengthPrefixedField_uint16BE(&msgIn->fields_publish.field_payload, payloadIn, payloadSize_bytesIn) ) return false;

	msgIn->areFieldsConfigured = true;
	return true;
}


bool cxa_mqtt_message_publish_getTopicName(cxa_mqtt_message_t *const msgIn, char** topicNameOut, uint16_t *const topicNameLen_bytesOut)
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

	if( payloadOut != NULL ) *payloadOut = payloadPtr;
	if( payloadSize_bytesOut != NULL ) *payloadSize_bytesOut = cxa_linkedField_getSize_bytes(&msgIn->fields_publish.field_payload);

	return true;
}


bool cxa_mqtt_message_publish_topicName_trimToPointer(cxa_mqtt_message_t *const msgIn, char *const ptrIn)
{
	cxa_assert(msgIn);
	cxa_assert(ptrIn);

	if( !msgIn->areFieldsConfigured || (cxa_mqtt_message_getType(msgIn) != CXA_MQTT_MSGTYPE_PUBLISH) ) return false;

	// get our topic name and make sure it's appropriate
	char* topicName;
	uint16_t topicNameLen_bytes;
	if( !cxa_mqtt_message_publish_getTopicName(msgIn, &topicName, &topicNameLen_bytes) ) return false;
	if( topicName > ptrIn ) return false;

	// make sure we don't go out of bounds
	uint16_t numBytesToTrimFromLeft = ptrIn - topicName;
	if( numBytesToTrimFromLeft > topicNameLen_bytes ) return false;

	return cxa_linkedField_removeFrom_lengthPrefixedField_uint16BE(&msgIn->fields_publish.field_topicName, 0, numBytesToTrimFromLeft);
}


bool cxa_mqtt_message_publish_topicName_prependCString(cxa_mqtt_message_t *const msgIn, char *const stringIn)
{
	cxa_assert(msgIn);
	cxa_assert(stringIn);

	if( !msgIn->areFieldsConfigured || (cxa_mqtt_message_getType(msgIn) != CXA_MQTT_MSGTYPE_PUBLISH) ) return false;

	return cxa_linkedField_prependTo_lengthPrefixedField_uint16BE(&msgIn->fields_publish.field_topicName, 0, (uint8_t*)stringIn, strlen(stringIn));
}


bool cxa_mqtt_message_publish_validateReceivedBytes(cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(msgIn);

	// first up is the topic name
	uint16_t numBytesInTopicName;
	if( !cxa_fixedByteBuffer_get_lengthPrefixedCString_uint16BE(msgIn->buffer, cxa_linkedField_getStartIndexOfNextField(&msgIn->field_remainingLength), NULL, &numBytesInTopicName, NULL) ||
			!cxa_linkedField_initChild(&msgIn->fields_publish.field_topicName, &msgIn->field_remainingLength, numBytesInTopicName+2) ) return false;

	// check our QOS level
	uint8_t packetTypeAndFlags;
	if( !cxa_linkedField_get_uint8(&msgIn->field_packetTypeAndFlags, 0, packetTypeAndFlags) ) return false;
	cxa_mqtt_qosLevel_t qos = (packetTypeAndFlags >> 1) & 0x03;
	cxa_linkedField_t* prevField = &msgIn->fields_publish.field_topicName;

	// packet id (if present)
	if( qos != CXA_MQTT_QOS_ATMOST_ONCE )
	{
		if( !cxa_linkedField_initChild_fixedLen(&msgIn->fields_publish.field_packetId, prevField, 2) ) return false;
		prevField = &msgIn->fields_publish.field_packetId;
	}

	// payload
	uint16_t numBytesInPayload = cxa_fixedByteBuffer_getSize_bytes(msgIn->buffer) - cxa_linkedField_getStartIndexOfNextField(prevField);
	if( !cxa_linkedField_initChild(&msgIn->fields_publish.field_payload, prevField, numBytesInPayload) ) return false;

	return true;
}


// ******** local function implementations ********
