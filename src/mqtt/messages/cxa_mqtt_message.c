/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_mqtt_message.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_mqtt_message_connect.h>
#include <cxa_mqtt_message_connack.h>
#include <cxa_mqtt_message_pingRequest.h>
#include <cxa_mqtt_message_pingResponse.h>
#include <cxa_mqtt_message_suback.h>
#include <cxa_mqtt_message_subscribe.h>
#include <cxa_mqtt_message_publish.h>

#define CXA_LOG_LEVEL				CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define REMAININGLEN_MAXBYTES 						4


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
cxa_mqtt_message_type_t cxa_mqtt_message_getType(cxa_mqtt_message_t *const msgIn)
{
	if( !msgIn->areFieldsConfigured ) return CXA_MQTT_MSGTYPE_UNKNOWN;

	uint8_t type_raw;
	if( !cxa_linkedField_get_uint8(&msgIn->field_packetTypeAndFlags, 0, type_raw) ) return CXA_MQTT_MSGTYPE_UNKNOWN;

	return cxa_mqtt_message_rxBytes_getType(type_raw);
}


cxa_fixedByteBuffer_t* cxa_mqtt_message_getBuffer(cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(msgIn);
	return msgIn->buffer;
}


void cxa_mqtt_message_initEmpty(cxa_mqtt_message_t *const msgIn, cxa_fixedByteBuffer_t *const fbbIn)
{
	cxa_assert(msgIn);

	// save our references
	msgIn->buffer = fbbIn;

	// set some defaults
	msgIn->areFieldsConfigured = false;
}


bool cxa_mqtt_message_validateReceivedBytes(cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(msgIn);

	// we need to set this temporarily so we can parse our fields as we go
	msgIn->areFieldsConfigured = true;

	// setup our linkedFields
	if( !cxa_linkedField_initRoot_fixedLen(&msgIn->field_packetTypeAndFlags, msgIn->buffer, 0, 1) ) { msgIn->areFieldsConfigured = false; return false; }

	// now our variable length field
	bool isVarLenComplete = false;
	size_t actualLen_bytes;
	size_t fieldLen_bytes;
	if( !cxa_mqtt_message_rxBytes_parseVariableLengthField(msgIn->buffer, &isVarLenComplete, &actualLen_bytes, &fieldLen_bytes) ||
			!isVarLenComplete ||
			(actualLen_bytes + 1 + fieldLen_bytes != cxa_fixedByteBuffer_getSize_bytes(msgIn->buffer)) ||
			!cxa_linkedField_initChild(&msgIn->field_remainingLength, &msgIn->field_packetTypeAndFlags, fieldLen_bytes) ) { msgIn->areFieldsConfigured = false; return false; }

	// check our message type
	cxa_mqtt_message_type_t msgType = cxa_mqtt_message_getType(msgIn);
	bool didMsgValidate = false;
	switch( msgType )
	{
		case CXA_MQTT_MSGTYPE_CONNECT:
			didMsgValidate = cxa_mqtt_message_connect_validateReceivedBytes(msgIn);
			break;

		case CXA_MQTT_MSGTYPE_CONNACK:
			didMsgValidate = cxa_mqtt_message_connack_validateReceivedBytes(msgIn);
			break;

		case CXA_MQTT_MSGTYPE_PUBLISH:
			didMsgValidate = cxa_mqtt_message_publish_validateReceivedBytes(msgIn);
			break;

		case CXA_MQTT_MSGTYPE_SUBSCRIBE:
			didMsgValidate = cxa_mqtt_message_subscribe_validateReceivedBytes(msgIn);
			break;

		case CXA_MQTT_MSGTYPE_SUBACK:
			didMsgValidate = cxa_mqtt_message_suback_validateReceivedBytes(msgIn);
			break;

		case CXA_MQTT_MSGTYPE_PINGREQ:
			didMsgValidate = cxa_mqtt_message_pingRequest_init(msgIn);
			break;

		case CXA_MQTT_MSGTYPE_PINGRESP:
			didMsgValidate = cxa_mqtt_message_pingResponse_init(msgIn);
			break;

		default:
			break;
	}
	if( !didMsgValidate ) { msgIn->areFieldsConfigured = false; return false; }

	return true;
}


cxa_mqtt_message_type_t cxa_mqtt_message_rxBytes_getType(uint8_t headerByteIn)
{
	uint8_t type_raw = (headerByteIn >> 4) & 0x0F;

	if( (type_raw != CXA_MQTT_MSGTYPE_CONNECT) &&
			(type_raw != CXA_MQTT_MSGTYPE_CONNACK) &&
			(type_raw != CXA_MQTT_MSGTYPE_PUBLISH) &&
			(type_raw != CXA_MQTT_MSGTYPE_SUBSCRIBE) &&
			(type_raw != CXA_MQTT_MSGTYPE_SUBACK) &&
			(type_raw != CXA_MQTT_MSGTYPE_PINGREQ) &&
			(type_raw != CXA_MQTT_MSGTYPE_PINGRESP) ) return CXA_MQTT_MSGTYPE_UNKNOWN;

	return (cxa_mqtt_message_type_t)type_raw;
}


bool cxa_mqtt_message_rxBytes_parseVariableLengthField(cxa_fixedByteBuffer_t *const fbbIn, bool *isCompleteOut, size_t *actualLengthOut, size_t *fieldLength_bytesOut)
{
	cxa_assert(fbbIn);

	size_t fbbLen_bytes = cxa_fixedByteBuffer_getSize_bytes(fbbIn);

	// make sure we have at least enough for one byte
	if( fbbLen_bytes < 2 )
	{
		if( isCompleteOut ) *isCompleteOut = false;
		return true;
	}

	// start calculating
	bool isComplete = false;
	uint32_t multiplier = 1;
	size_t value = 0;
	size_t i;
	for( i = 1; i < fbbLen_bytes; i++ )
	{
		uint8_t currByte;
		if( !cxa_fixedByteBuffer_get_uint8(fbbIn, i, currByte) )
		{
			// not enough bytes
			if( isCompleteOut ) *isCompleteOut = false;
			return true;
		}

		value += (currByte & 0x7F) * multiplier;
		multiplier *= 128;
		if( multiplier > ((uint32_t)128) * ((uint32_t)128) * ((uint32_t)128) )
		{
			// malformed length field
			return false;
		}

		// see if this is the end
		if( !(currByte & 0x80) )
		{
			// this is the last byte
			isComplete = true;
			break;
		}
	}

	if( isCompleteOut ) *isCompleteOut = isComplete;
	if( isComplete && actualLengthOut ) *actualLengthOut = value;
	if( isComplete && fieldLength_bytesOut ) *fieldLength_bytesOut = i;

	return true;
}


#define CXA_LOG_LEVEL				CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>

bool cxa_mqtt_message_updateVariableLengthField(cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(msgIn);

	if( !msgIn->areFieldsConfigured ) return false;

	// clear our existing length field
	if( !cxa_linkedField_clear(&msgIn->field_remainingLength) ) return false;

	// recalculate...total length - first fixed header byte(1) - us(now 0)
	size_t remainingLength_actual = cxa_fixedByteBuffer_getSize_bytes(msgIn->buffer) - 1;

	// convert to variable length encoding
	uint8_t varLenBytes[REMAININGLEN_MAXBYTES];
	size_t numBytes_varLenField = 0;
	do
	{
		uint8_t currByte = remainingLength_actual % 128;
		remainingLength_actual = remainingLength_actual / 128;
		// if there are more data to encode, set the top bit of this byte
		if( remainingLength_actual > 0 ) currByte |= 128;

		varLenBytes[numBytes_varLenField++] = currByte;

		if( numBytes_varLenField >= REMAININGLEN_MAXBYTES ) return false;
	} while(remainingLength_actual > 0);

	return cxa_linkedField_append(&msgIn->field_remainingLength, varLenBytes, numBytes_varLenField);
}


// ******** local function implementations ********
