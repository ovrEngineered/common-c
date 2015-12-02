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
#include "cxa_mqtt_message_subscribe.h"


// ******** includes ********
#include <string.h>

#include <cxa_assert.h>
#include <cxa_linkedField.h>

#define CXA_LOG_LEVEL				CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
bool cxa_mqtt_message_subscribe_init(cxa_mqtt_message_t *const msgIn, uint16_t packetIdIn, char *const topicFilterIn, cxa_mqtt_qosLevel_t qosLevelIn)
{
	cxa_assert(msgIn);
	cxa_assert(topicFilterIn)

	// fixed header 1
	if( !cxa_linkedField_initRoot_fixedLen(&msgIn->field_packetTypeAndFlags, msgIn->buffer, 0, 1) ||
			!cxa_linkedField_append_uint8(&msgIn->field_packetTypeAndFlags, ((CXA_MQTT_MSGTYPE_SUBSCRIBE << 4) | 0x02)) ) return false;

	// remaining length
	if( !cxa_linkedField_initChild(&msgIn->field_remainingLength, &msgIn->field_packetTypeAndFlags, 0) ) return false;

	// packet id
	if( !cxa_linkedField_initChild_fixedLen(&msgIn->fields_subscribe.field_packetId, &msgIn->field_remainingLength, 2) ||
				!cxa_linkedField_append_uint16BE(&msgIn->fields_subscribe.field_packetId, packetIdIn) ) return false;

	// topic filter
	if( !cxa_linkedField_initChild(&msgIn->fields_subscribe.field_topicFilter, &msgIn->fields_subscribe.field_packetId, 0) ||
			!cxa_linkedField_append_lengthPrefixedCString_uint16BE(&msgIn->fields_subscribe.field_topicFilter, topicFilterIn, false) ) return false;

	// qos
	if( !cxa_linkedField_initChild_fixedLen(&msgIn->fields_subscribe.field_qos, &msgIn->fields_subscribe.field_topicFilter, 1) ||
			!cxa_linkedField_append_uint8(&msgIn->fields_subscribe.field_qos, qosLevelIn) ) return false;

	msgIn->areFieldsConfigured = true;
	return true;
}


bool cxa_mqtt_message_subscribe_validateReceivedBytes(cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(msgIn);

	// first up is the topic filter
	uint16_t numBytesInTopicFilter;
	if( !cxa_fixedByteBuffer_get_lengthPrefixedCString_uint16BE(msgIn->buffer, cxa_linkedField_getStartIndexOfNextField(&msgIn->field_remainingLength), NULL, &numBytesInTopicFilter, NULL) ||
			!cxa_linkedField_initChild(&msgIn->fields_subscribe.field_topicFilter, &msgIn->field_remainingLength, numBytesInTopicFilter+2) ) return false;

	// next is the qos
	if( !cxa_linkedField_initChild_fixedLen(&msgIn->fields_subscribe.field_qos, &msgIn->fields_subscribe.field_topicFilter, 1) ) return false;

	return true;
}


// ******** local function implementations ********
