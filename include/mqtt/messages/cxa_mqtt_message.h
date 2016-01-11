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
#ifndef CXA_MQTT_MESSAGE_H_
#define CXA_MQTT_MESSAGE_H_


// ******** includes ********
#include <cxa_fixedByteBuffer.h>
#include <cxa_linkedField.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct cxa_mqtt_message cxa_mqtt_message_t;


typedef enum
{
	CXA_MQTT_MSGTYPE_CONNECT=1,
	CXA_MQTT_MSGTYPE_CONNACK=2,
	CXA_MQTT_MSGTYPE_PUBLISH=3,
	CXA_MQTT_MSGTYPE_SUBSCRIBE=8,
	CXA_MQTT_MSGTYPE_SUBACK=9,
	CXA_MQTT_MSGTYPE_PINGREQ=12,
	CXA_MQTT_MSGTYPE_PINGRESP=13,
	CXA_MQTT_MSGTYPE_UNKNOWN=255
}cxa_mqtt_message_type_t;


typedef enum
{
	CXA_MQTT_QOS_ATMOST_ONCE=0,
	//CXA_MQTT_QOS_ATLEAST_ONCE=1, -- not supported
	//CXA_MQTT_QOS_EXACTLY_ONCE=2 -- not supported
}cxa_mqtt_qosLevel_t;


struct cxa_mqtt_message
{
	cxa_fixedByteBuffer_t* buffer;

	bool areFieldsConfigured;
	cxa_linkedField_t field_packetTypeAndFlags;
	cxa_linkedField_t field_remainingLength;

	struct
	{
		cxa_linkedField_t field_protocol;
		cxa_linkedField_t field_protocolLevel;
		cxa_linkedField_t field_connectFlags;
		cxa_linkedField_t field_keepAlive;
		cxa_linkedField_t field_clientId;

		cxa_linkedField_t field_willTopic;
		cxa_linkedField_t field_willMessage;

		cxa_linkedField_t field_username;
		cxa_linkedField_t field_password;
	}fields_connect;

	struct
	{
		cxa_linkedField_t field_sessionPresent;
		cxa_linkedField_t field_returnCode;
	}fields_connack;

	struct
	{
		cxa_linkedField_t field_packetId;
		cxa_linkedField_t field_topicFilter;
		cxa_linkedField_t field_qos;
	}fields_subscribe;

	struct
	{
		cxa_linkedField_t field_packetId;
		cxa_linkedField_t field_returnCode;
	}fields_suback;

	struct
	{
		cxa_linkedField_t field_topicName;
		cxa_linkedField_t field_packetId;
		cxa_linkedField_t field_payload;
	}fields_publish;
};


// ******** global function prototypes ********
/**
 * @public
 */
cxa_mqtt_message_type_t cxa_mqtt_message_getType(cxa_mqtt_message_t *const msgIn);


/**
 * @public
 */
cxa_fixedByteBuffer_t* cxa_mqtt_message_getBuffer(cxa_mqtt_message_t *const msgIn);


/**
 * @protected
 */
void cxa_mqtt_message_initEmpty(cxa_mqtt_message_t *const msgIn, cxa_fixedByteBuffer_t *const fbbIn);


/**
 * @protected
 */
bool cxa_mqtt_message_validateReceivedBytes(cxa_mqtt_message_t *const msgIn);


/**
 * @protected
 */
cxa_mqtt_message_type_t cxa_mqtt_message_rxBytes_getType(uint8_t headerByteIn);


/**
 * @protected
 * @return
 */
bool cxa_mqtt_message_rxBytes_parseVariableLengthField(cxa_fixedByteBuffer_t *const fbbIn, bool *isCompleteOut, size_t *actualLengthOut, size_t *fieldLength_bytesOut);


/**
 * @protected
 */
bool cxa_mqtt_message_updateVariableLengthField(cxa_mqtt_message_t *const msgIn);


#endif /* CXA_MQTT_MESSAGE_H_ */
