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
#include "cxa_mqtt_message_connect.h"


// ******** includes ********
#include <string.h>

#include <cxa_assert.h>
#include <cxa_linkedField.h>

#define CXA_LOG_LEVEL				CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define PROTOCOL_LEVEL				4


// ******** local type definitions ********


// ******** local function prototypes ********
static bool getConnectFlags(cxa_mqtt_message_t *const msgIn, uint8_t *const connectFlagsOut);


// ********  local variable declarations *********


// ******** global function implementations ********
bool cxa_mqtt_message_connect_init(cxa_mqtt_message_t *const msgIn, char* clientIdIn, char* usernameIn, uint8_t* passwordIn, uint16_t passwordLen_bytesIn, bool cleanSessionIn, uint16_t keepAlive_sIn)
{
	cxa_assert(msgIn);
	cxa_assert(clientIdIn);

	// get the clientId length
	size_t cidLen_bytes = strlen(clientIdIn);
	cxa_assert( (cidLen_bytes >= 1) && (cidLen_bytes <= 23) );

	// fixed header 1
	if( !cxa_linkedField_initRoot_fixedLen(&msgIn->field_packetTypeAndFlags, msgIn->buffer, 0, 1) ||
			!cxa_linkedField_append_uint8(&msgIn->field_packetTypeAndFlags, (CXA_MQTT_MSGTYPE_CONNECT << 4) ) ) return false;

	// remaining length
	if( !cxa_linkedField_initChild(&msgIn->field_remainingLength, &msgIn->field_packetTypeAndFlags, 0) ) return false;

	// protocol name
	if( !cxa_linkedField_initChild(&msgIn->fields_connect.field_protocol, &msgIn->field_remainingLength, 0) ||
			!cxa_linkedField_append_lengthPrefixedCString_uint16BE(&msgIn->fields_connect.field_protocol, "MQTT", false) ) return false;

	// protocol level
	if( !cxa_linkedField_initChild_fixedLen(&msgIn->fields_connect.field_protocolLevel, &msgIn->fields_connect.field_protocol, 1) ||
			!cxa_linkedField_append_uint8(&msgIn->fields_connect.field_protocolLevel, PROTOCOL_LEVEL) ) return false;

	// connect flags
	uint8_t connectFlags = ((usernameIn != NULL) ? 0x80 : 0x00) | ((passwordIn != NULL) ? 0x40 : 0x00) | (cleanSessionIn ? 0x02 :0x00);
	if( !cxa_linkedField_initChild_fixedLen(&msgIn->fields_connect.field_connectFlags, &msgIn->fields_connect.field_protocolLevel, 1) ||
				!cxa_linkedField_append_uint8(&msgIn->fields_connect.field_connectFlags, connectFlags) ) return false;

	// keepalive
	if( !cxa_linkedField_initChild_fixedLen(&msgIn->fields_connect.field_keepAlive, &msgIn->fields_connect.field_connectFlags, 2) ||
				!cxa_linkedField_append_uint16BE(&msgIn->fields_connect.field_keepAlive, keepAlive_sIn) ) return false;

	// client id
	if( !cxa_linkedField_initChild(&msgIn->fields_connect.field_clientId, &msgIn->fields_connect.field_keepAlive, 0) ||
				!cxa_linkedField_append_lengthPrefixedCString_uint16BE(&msgIn->fields_connect.field_clientId, clientIdIn, false) ) return false;
	cxa_linkedField_t* prevField = &msgIn->fields_connect.field_clientId;

	// @TODO will topic and message (if present)

	// username (if present)
	if( usernameIn != NULL )
	{
		if( !cxa_linkedField_initChild(&msgIn->fields_connect.field_username, prevField, 0) ||
						!cxa_linkedField_append_lengthPrefixedCString_uint16BE(&msgIn->fields_connect.field_username, usernameIn, false) ) return false;
		prevField = &msgIn->fields_connect.field_username;
	}

	// password (if present)
	if( passwordIn != NULL )
	{
		if( !cxa_linkedField_initChild(&msgIn->fields_connect.field_password, prevField, 0) ||
						!cxa_linkedField_append_lengthPrefixedField_uint16BE(&msgIn->fields_connect.field_password, passwordIn, passwordLen_bytesIn) ) return false;
		prevField = &msgIn->fields_connect.field_password;
	}

	msgIn->areFieldsConfigured = true;
	return true;
}


bool cxa_mqtt_message_connect_hasWill(cxa_mqtt_message_t *const msgIn, bool *const hasWillOut)
{
	cxa_assert(msgIn);

	uint8_t connectFlags;
	if( !getConnectFlags(msgIn, &connectFlags) ) return false;

	if( hasWillOut != NULL ) *hasWillOut = connectFlags & 0x04;
	return true;
}


bool cxa_mqtt_message_connect_hasUsername(cxa_mqtt_message_t *const msgIn, bool *const hasUsernameOut)
{
	cxa_assert(msgIn);

	uint8_t connectFlags;
	if( !getConnectFlags(msgIn, &connectFlags) ) return false;

	if( hasUsernameOut != NULL ) *hasUsernameOut = connectFlags & 0x80;
	return true;
}


bool cxa_mqtt_message_connect_hasPassword(cxa_mqtt_message_t *const msgIn, bool *const hasPasswordOut)
{
	cxa_assert(msgIn);

	uint8_t connectFlags;
	if( !getConnectFlags(msgIn, &connectFlags) ) return false;

	if( hasPasswordOut != NULL ) *hasPasswordOut = connectFlags & 0x40;
	return true;
}


bool cxa_mqtt_message_connect_cleanSessionRequested(cxa_mqtt_message_t *const msgIn, bool *const cleanSessionRequestedOut)
{
	cxa_assert(msgIn);

	uint8_t connectFlags;
	if( !getConnectFlags(msgIn, &connectFlags) ) return false;

	if( cleanSessionRequestedOut != NULL ) *cleanSessionRequestedOut = connectFlags & 0x02;
	return true;
}


bool cxa_mqtt_message_connect_validateReceivedBytes(cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(msgIn);

	// first up is the protocol name
	char* protocolName;
	uint16_t numBytesInProtocolName;
	if( !cxa_fixedByteBuffer_get_lengthPrefixedCString_uint16BE(msgIn->buffer, cxa_linkedField_getStartIndexOfNextField(&msgIn->field_remainingLength), &protocolName, &numBytesInProtocolName, NULL) ||
			(numBytesInProtocolName != 4) ||
			(strncmp(protocolName, "MQTT", numBytesInProtocolName) != 0) ) { return false; }
	if( !cxa_linkedField_initChild(&msgIn->fields_connect.field_protocol, &msgIn->field_remainingLength, numBytesInProtocolName+2) ) return false;

	// next is the protocol level
	if( !cxa_linkedField_initChild_fixedLen(&msgIn->fields_connect.field_protocolLevel, &msgIn->fields_connect.field_protocol, 1) ) return false;

	// next is the connect flags
	if( !cxa_linkedField_initChild_fixedLen(&msgIn->fields_connect.field_connectFlags, &msgIn->fields_connect.field_protocolLevel, 1) ) return false;

	// next is the keepalive
	if( !cxa_linkedField_initChild_fixedLen(&msgIn->fields_connect.field_keepAlive, &msgIn->fields_connect.field_connectFlags, 2) ) return false;

	// now the client id
	uint16_t numBytesInClientId;
	if( !cxa_fixedByteBuffer_get_lengthPrefixedCString_uint16BE(msgIn->buffer, cxa_linkedField_getStartIndexOfNextField(&msgIn->fields_connect.field_keepAlive), NULL, &numBytesInClientId, NULL) ) return false;
	if( !cxa_linkedField_initChild(&msgIn->fields_connect.field_clientId, &msgIn->fields_connect.field_keepAlive, numBytesInClientId+2) ) return false;

	// now the will topic and message (if present)
	cxa_linkedField_t* prevField = &msgIn->fields_connect.field_clientId;
	bool hasWill;
	if( !cxa_mqtt_message_connect_hasWill(msgIn, &hasWill) ) return false;
	if( hasWill )
	{
		uint16_t numBytesInWillTopic;
		if( !cxa_fixedByteBuffer_get_lengthPrefixedCString_uint16BE(msgIn->buffer, cxa_linkedField_getStartIndexOfNextField(prevField), NULL, &numBytesInWillTopic, NULL) ) return false;
		if( !cxa_linkedField_initChild(&msgIn->fields_connect.field_willTopic, prevField, numBytesInWillTopic+2) ) return false;
		prevField = &msgIn->fields_connect.field_willTopic;

		uint16_t numBytesInWillMessage;
		if( !cxa_fixedByteBuffer_get_lengthPrefixedField_uint16BE(msgIn->buffer, cxa_linkedField_getStartIndexOfNextField(prevField), NULL, &numBytesInWillMessage) ) return false;
		if( !cxa_linkedField_initChild(&msgIn->fields_connect.field_willMessage, prevField, numBytesInWillMessage+2) ) return false;
		prevField = &msgIn->fields_connect.field_willMessage;
	}

	// now the username (if present)
	bool hasUsername;
	if( !cxa_mqtt_message_connect_hasUsername(msgIn, &hasUsername) ) return false;
	if( hasUsername )
	{
		uint16_t numBytesInUsername;
		if( !cxa_fixedByteBuffer_get_lengthPrefixedCString_uint16BE(msgIn->buffer, cxa_linkedField_getStartIndexOfNextField(prevField), NULL, &numBytesInUsername, NULL) ) return false;
		if( !cxa_linkedField_initChild(&msgIn->fields_connect.field_username, prevField, numBytesInUsername+2) ) return false;
		prevField = &msgIn->fields_connect.field_username;
	}

	// now the password (if present)
	bool hasPassword;
	if( !cxa_mqtt_message_connect_hasPassword(msgIn, &hasPassword) ) return false;
	if( hasPassword )
	{
		uint16_t numBytesInPassword;
		if( !cxa_fixedByteBuffer_get_lengthPrefixedCString_uint16BE(msgIn->buffer, cxa_linkedField_getStartIndexOfNextField(prevField), NULL, &numBytesInPassword, NULL) ) return false;
		if( !cxa_linkedField_initChild(&msgIn->fields_connect.field_password, prevField, numBytesInPassword+2) ) return false;
		prevField = &msgIn->fields_connect.field_password;
	}

	return true;
}


// ******** local function implementations ********
static bool getConnectFlags(cxa_mqtt_message_t *const msgIn, uint8_t *const connectFlagsOut)
{
	cxa_assert(msgIn);

	if( !msgIn->areFieldsConfigured || (cxa_mqtt_message_getType(msgIn) != CXA_MQTT_MSGTYPE_CONNECT) ) return false;

	uint8_t connectFlags_lcl;
	if( !cxa_linkedField_get_uint8(&msgIn->fields_connect.field_connectFlags, 0, connectFlags_lcl) ) return false;

	if( connectFlagsOut != NULL ) *connectFlagsOut = connectFlags_lcl;
	return true;
}
