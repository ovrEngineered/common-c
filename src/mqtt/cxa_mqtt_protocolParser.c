/**
 * Copyright 2015 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Christopher Armenio
 */
#include "cxa_mqtt_protocolParser.h"


// ******** includes ********
#include <string.h>
#include <cxa_assert.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define REMAININGLEN_MAXBYTES		4
#define ERR_FBB_OVERFLOW			"fbb overflow"
#define ERR_MALFORMED_PACKET		"malformed packet"
#define ERR_MALFORMED_HEADER		"malformed header"


// ******** local type definitions ********
typedef enum
{
	PROTO_STATE_WAIT_FIXEDHEADER_1,
	PROTO_STATE_WAIT_REMAINING_LEN,
	PROTO_STATE_WAIT_DATABYTES,
	PROTO_STATE_PROCESS_PACKET
}state_t;

typedef struct{
	cxa_mqtt_protocolParser_connAck_returnCode_t retCode;
	char string[];
}connAck_retCode_stringMap_entry_t;


// ******** local function prototypes ********
static void stateCb_waitFixedHeader1_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_waitFixedHeader1_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_waitRemainingLen_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_waitDataBytes_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_processPacket_enter(cxa_stateMachine_t *const smIn, void *userVarIn);

static void processPacket_connAck(cxa_mqtt_protocolParser_t *const mppIn);
static void processPacket_pingResp(cxa_mqtt_protocolParser_t *const mppIn);
static void processPacket_subAck(cxa_mqtt_protocolParser_t *const mppIn);
static void processPacket_publish(cxa_mqtt_protocolParser_t *const mppIn);

static bool processVariableLengthField(cxa_fixedByteBuffer_t *const fbbIn, bool *isCompleteOut, size_t *actualLengthOut, size_t *fieldLength_bytesOut);

static bool serialize_lengthAsVariableLengthEncoding(cxa_ioStream_t *iosIn, size_t packetLen_bytesIn);
static bool serialize_cString(cxa_ioStream_t *iosIn, char *const stringIn);


// ********  local variable declarations *********
static connAck_retCode_stringMap_entry_t connAck_retCode_stringMap [] = {
	{CXA_MQTT_CONNACK_RETCODE_ACCEPTED, "accepted"},
	{CXA_MQTT_CONNACK_RETCODE_REFUSED_PROTO, "bad protocol level"},
	{CXA_MQTT_CONNACK_RETCODE_REFUSED_CID, "bad clientId"},
	{CXA_MQTT_CONNACK_RETCODE_REFUSED_SERVERUNAVAILABLE, "mqtt unavailable"},
	{CXA_MQTT_CONNACK_RETCODE_REFUSED_BADUSERNAMEPASSWORD, "malformed username/password"},
	{CXA_MQTT_CONNACK_RETCODE_REFUSED_NOTAUTHORIZED, "client not authorized"}
};

// ******** global function implementations ********
void cxa_mqtt_protocolParser_init(cxa_mqtt_protocolParser_t *const mppIn, cxa_ioStream_t *const ioStreamIn)
{
	cxa_assert(mppIn);
	cxa_assert(ioStreamIn);

	// save our references
	mppIn->ioStream = ioStreamIn;

	// set some default values
	mppIn->remainingBytesToReceive = 0;

	// setup our listeners array
	cxa_array_initStd(&mppIn->listeners, mppIn->listeners_raw);

	// setup our RX fixedByteBuffer
	cxa_fixedByteBuffer_initStd(&mppIn->fbb, mppIn->fbb_raw);

	// setup our logger
	cxa_logger_init(&mppIn->logger, "mqttProtoParser");

	// setup our state machine
	cxa_stateMachine_init(&mppIn->stateMachine, "mqttProtoParser");
	cxa_stateMachine_addState(&mppIn->stateMachine, PROTO_STATE_WAIT_FIXEDHEADER_1, "wait_fh1", stateCb_waitFixedHeader1_enter, stateCb_waitFixedHeader1_state, NULL, (void*)mppIn);
	cxa_stateMachine_addState(&mppIn->stateMachine, PROTO_STATE_WAIT_REMAINING_LEN, "wait_remLen", NULL, stateCb_waitRemainingLen_state, NULL, (void*)mppIn);
	cxa_stateMachine_addState(&mppIn->stateMachine, PROTO_STATE_WAIT_DATABYTES, "wait_dataBytes", NULL, stateCb_waitDataBytes_state, NULL, (void*)mppIn);
	cxa_stateMachine_addState(&mppIn->stateMachine, PROTO_STATE_PROCESS_PACKET, "processPacket", stateCb_processPacket_enter, NULL, NULL, (void*)mppIn);
	cxa_stateMachine_transition(&mppIn->stateMachine, PROTO_STATE_WAIT_FIXEDHEADER_1);
	cxa_stateMachine_update(&mppIn->stateMachine);
}


void cxa_mqtt_protocolParser_addListener(cxa_mqtt_protocolParser_t *const mppIn,
										 cxa_mqtt_protocolParser_cb_onConnAck_t cb_onConnAckIn,
										 cxa_mqtt_protocolParser_cb_onPingResp_t cb_onPingResp,
										 cxa_mqtt_protocolParser_cb_onSubAck_t cb_onSubAckIn,
										 cxa_mqtt_protocolParser_cb_onPublish_t cb_onPublishIn,
										 void *const userVarIn)
{
	cxa_assert(mppIn);

	cxa_mqtt_protocolParser_listenerEntry_t newEntry = {.cb_onConnAck=cb_onConnAckIn,
			.cb_onPingResp=cb_onPingResp,
			.cb_onSubAck=cb_onSubAckIn,
			.cb_onPublish=cb_onPublishIn,
			.userVar=userVarIn};
	cxa_assert( cxa_array_append(&mppIn->listeners, &newEntry) );
}


bool cxa_mqtt_protocolParser_writePacket_connect(cxa_mqtt_protocolParser_t *const mppIn, char* clientIdIn, char* usernameIn, char* passwordIn, bool cleanSessionIn, uint16_t keepAlive_sIn)
{
	cxa_assert(mppIn);
	cxa_assert(clientIdIn);

	// get the clientId length
	size_t cidLen_bytes = strlen(clientIdIn);
	cxa_assert( (cidLen_bytes >= 1) && (cidLen_bytes <= 23) );

	// let's calculate our length first
	size_t packetLen_bytes = 0;
	packetLen_bytes += 6;				// variable header
	packetLen_bytes += 1;				// protocol level
	packetLen_bytes += 1; 				// connect flags
	packetLen_bytes += 2;				// keep alive
	packetLen_bytes += 2+cidLen_bytes;	// clientId
	// no will fields
	if( usernameIn != NULL ) packetLen_bytes += 2+strlen(usernameIn);
	if( passwordIn != NULL ) packetLen_bytes += 2+strlen(passwordIn);

	// start outputting bytes...

	// fixed packet header
	if( !cxa_ioStream_writeByte(mppIn->ioStream, (CXA_MQTT_PACKETTYPE_CONNECT << 4)) ) return false;
	if( !serialize_lengthAsVariableLengthEncoding(mppIn->ioStream, packetLen_bytes) ) return false;

	// variable header
	if( !cxa_ioStream_writeByte(mppIn->ioStream, 0x00) ) return false;
	if( !cxa_ioStream_writeByte(mppIn->ioStream, 0x04) ) return false;
	if( !cxa_ioStream_writeString(mppIn->ioStream, "MQTT") ) return false;

	// protocol level
	if( !cxa_ioStream_writeByte(mppIn->ioStream, 0x04) ) return false;

	// connect flags
	if( !cxa_ioStream_writeByte(mppIn->ioStream, ((usernameIn != NULL) << 7) | ((passwordIn != NULL) << 6) | cleanSessionIn << 1) ) return false;

	// keep alive
	if( !cxa_ioStream_writeByte(mppIn->ioStream, ((keepAlive_sIn >> 8) & 0x00FF)) ) return false;
	if( !cxa_ioStream_writeByte(mppIn->ioStream, ((keepAlive_sIn >> 0) & 0x00FF)) ) return false;

	// payload - client identifier
	if( !serialize_cString(mppIn->ioStream, clientIdIn) ) return false;

	// payload - no will fields

	// payload - username
	if( (usernameIn != NULL) && !serialize_cString(mppIn->ioStream, usernameIn) ) return false;

	// payload - password
	if( (passwordIn != NULL) && !serialize_cString(mppIn->ioStream, passwordIn) ) return false;

	return true;
}


bool cxa_mqtt_protocolParser_writePacket_pingReq(cxa_mqtt_protocolParser_t *const mppIn)
{
	cxa_assert(mppIn);

	// start outputting bytes...

	// fixed packet header
	if( !cxa_ioStream_writeByte(mppIn->ioStream, (CXA_MQTT_PACKETTYPE_PINGREQ << 4)) ) return false;
	if( !serialize_lengthAsVariableLengthEncoding(mppIn->ioStream, 0) ) return false;

	return true;
}


bool cxa_mqtt_protocolParser_writePacket_publish(cxa_mqtt_protocolParser_t *const mppIn, cxa_mqtt_protocolParser_qosLevel_t qosIn, bool retainIn,
												 char* topicNameIn, void *const payloadIn, size_t payloadLen_bytesIn)
{
	cxa_assert(mppIn);
	if( payloadLen_bytesIn > 0 ) cxa_assert(payloadIn);

	// depends on our QOS level
	switch( qosIn )
	{
		case CXA_MQTT_QOS_ATMOST_ONCE:
		{
			// let's calculate our length first
			size_t packetLen_bytes = 0;
			packetLen_bytes += 2+strlen(topicNameIn);
			// no packet identifier
			packetLen_bytes += payloadLen_bytesIn;

			// start outputting bytes...

			// fixed packet header
			if( !cxa_ioStream_writeByte(mppIn->ioStream, (CXA_MQTT_PACKETTYPE_PUBLISH << 4) | retainIn) ) return false;
			if( !serialize_lengthAsVariableLengthEncoding(mppIn->ioStream, packetLen_bytes) ) return false;

			// variable header (topic name)
			if( !serialize_cString(mppIn->ioStream, topicNameIn) ) return false;

			// no packet identifier

			// our payload
			if( !cxa_ioStream_writeBytes(mppIn->ioStream, payloadIn, payloadLen_bytesIn) ) return false;

			return true;
			break;
		}

		default:
			break;
	}

	// if we made it here, we failed
	return false;
}


bool cxa_mqtt_protocolParser_writePacket_subscribe(cxa_mqtt_protocolParser_t *const mppIn, uint16_t packetIdIn, char *topicFilterIn, cxa_mqtt_protocolParser_qosLevel_t qosIn)
{
	cxa_assert(mppIn);
	cxa_assert(topicFilterIn);

	// let's calculate our length first
	size_t packetLen_bytes = 0;
	packetLen_bytes += 2;							// packetId
	packetLen_bytes += 2+strlen(topicFilterIn);		// topic filter
	packetLen_bytes += 1;							// qos for topic filter

	// start outputting bytes...

	// fixed packet header
	if( !cxa_ioStream_writeByte(mppIn->ioStream, (CXA_MQTT_PACKETTYPE_SUBSCRIBE << 4) | 0x02) ) return false;
	if( !serialize_lengthAsVariableLengthEncoding(mppIn->ioStream, packetLen_bytes) ) return false;

	// variable header (packetId)
	if( !cxa_ioStream_writeByte(mppIn->ioStream, (packetIdIn >> 8) & 0x00FF) ) return false;
	if( !cxa_ioStream_writeByte(mppIn->ioStream, (packetIdIn >> 0) & 0x00FF) ) return false;

	// topic filter and QOS pair
	if( !serialize_cString(mppIn->ioStream, topicFilterIn) ) return false;
	if( !cxa_ioStream_writeByte(mppIn->ioStream, qosIn) ) return false;

	return true;
}


void cxa_mqtt_protocolParser_update(cxa_mqtt_protocolParser_t *const mppIn)
{
	cxa_assert(mppIn);

	cxa_stateMachine_update(&mppIn->stateMachine);
}


char* cxa_mqtt_protocolParser_getStringForConnAckRetCode(cxa_mqtt_protocolParser_connAck_returnCode_t retCodeIn)
{
	for( int i = 0; i < sizeof(connAck_retCode_stringMap)/sizeof(*connAck_retCode_stringMap); i++ )
	{
		connAck_retCode_stringMap_entry_t* currEntry = &connAck_retCode_stringMap[i];
		if( currEntry->retCode == retCodeIn ) return currEntry->string;
	}
	return "unknown";
}


// ******** local function implementations ********
static void stateCb_waitFixedHeader1_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_mqtt_protocolParser_t *mppIn = (cxa_mqtt_protocolParser_t*)userVarIn;
	cxa_assert(mppIn);

	// clear existing data in our buffer
	cxa_fixedByteBuffer_clear(&mppIn->fbb);
}


static void stateCb_waitFixedHeader1_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_mqtt_protocolParser_t *mppIn = (cxa_mqtt_protocolParser_t*)userVarIn;
	cxa_assert(mppIn);

	uint8_t rxByte;
	if( cxa_ioStream_readByte(mppIn->ioStream, &rxByte) == CXA_IOSTREAM_READSTAT_GOTDATA )
	{
		cxa_mqtt_protocolParser_packetType_t packetType = (rxByte >> 4) & 0x0F;
		switch( packetType )
		{
			case CXA_MQTT_PACKETTYPE_CONNECT:
			case CXA_MQTT_PACKETTYPE_CONNACK:
			case CXA_MQTT_PACKETTYPE_PINGREQ:
			case CXA_MQTT_PACKETTYPE_PINGRESP:
			case CXA_MQTT_PACKETTYPE_SUBACK:
				// make sure the flags match
				if( (rxByte & 0x0F) == 0 )
				{
					if( cxa_fixedByteBuffer_append_uint8(&mppIn->fbb, rxByte) )
					{
						cxa_stateMachine_transition(&mppIn->stateMachine, PROTO_STATE_WAIT_REMAINING_LEN);
						return;
					}
					else cxa_logger_warn(&mppIn->logger, ERR_FBB_OVERFLOW);
				} else cxa_logger_warn(&mppIn->logger, ERR_MALFORMED_HEADER);
				break;

			case CXA_MQTT_PACKETTYPE_PUBLISH:
				// flags don't matter for this one (can be anything)
				if( cxa_fixedByteBuffer_append_uint8(&mppIn->fbb, rxByte) )
				{
					cxa_stateMachine_transition(&mppIn->stateMachine, PROTO_STATE_WAIT_REMAINING_LEN);
					return;
				}
				else cxa_logger_warn(&mppIn->logger, ERR_FBB_OVERFLOW);
				break;

			default:
				cxa_logger_warn(&mppIn->logger, "unknown header byte: 0x%02X", rxByte);
				break;
		}
	}
}


static void stateCb_waitRemainingLen_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_mqtt_protocolParser_t *mppIn = (cxa_mqtt_protocolParser_t*)userVarIn;
	cxa_assert(mppIn);

	uint8_t rxByte;
	if( cxa_ioStream_readByte(mppIn->ioStream, &rxByte) == CXA_IOSTREAM_READSTAT_GOTDATA )
	{
		// add to our buffer
		if( !cxa_fixedByteBuffer_append_uint8(&mppIn->fbb, rxByte) )
		{
			cxa_logger_warn(&mppIn->logger, ERR_FBB_OVERFLOW);
			cxa_stateMachine_transition(&mppIn->stateMachine, PROTO_STATE_WAIT_FIXEDHEADER_1);
			return;
		}

		// process our variable length field (or the fraction we currently have)
		bool isVarLengthComplete;
		size_t actualLength;
		if( !processVariableLengthField(&mppIn->fbb, &isVarLengthComplete, &actualLength, NULL) )
		{
			cxa_logger_warn(&mppIn->logger, ERR_MALFORMED_HEADER);
			cxa_stateMachine_transition(&mppIn->stateMachine, PROTO_STATE_WAIT_FIXEDHEADER_1);
			return;
		}

		if( isVarLengthComplete )
		{
			mppIn->remainingBytesToReceive = actualLength;
			cxa_logger_trace(&mppIn->logger, "waiting for %d bytes", mppIn->remainingBytesToReceive);
			cxa_stateMachine_transition(&mppIn->stateMachine, PROTO_STATE_WAIT_DATABYTES);
			return;
		}
	}
}


static void stateCb_waitDataBytes_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_mqtt_protocolParser_t *mppIn = (cxa_mqtt_protocolParser_t*)userVarIn;
	cxa_assert(mppIn);

	// see if we've gotten enough bytes yet...
	if( mppIn->remainingBytesToReceive == 0 )
	{
		cxa_stateMachine_transition(&mppIn->stateMachine, PROTO_STATE_PROCESS_PACKET);
		return;
	}

	// keep receiving bytes
	uint8_t rxByte;
	if( cxa_ioStream_readByte(mppIn->ioStream, &rxByte) == CXA_IOSTREAM_READSTAT_GOTDATA )
	{
		// add to our buffer
		if( !cxa_fixedByteBuffer_append_uint8(&mppIn->fbb, rxByte) )
		{
			cxa_logger_warn(&mppIn->logger, ERR_FBB_OVERFLOW);
			cxa_stateMachine_transition(&mppIn->stateMachine, PROTO_STATE_WAIT_FIXEDHEADER_1);
			return;
		}
		mppIn->remainingBytesToReceive--;
	}
}


static void stateCb_processPacket_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_mqtt_protocolParser_t *mppIn = (cxa_mqtt_protocolParser_t*)userVarIn;
	cxa_assert(mppIn);

	// figure out what kind of packet this is
	uint8_t fixedHeader_1;
	if( !cxa_fixedByteBuffer_get_uint8(&mppIn->fbb, 0, fixedHeader_1) )
	{
		cxa_logger_warn(&mppIn->logger, ERR_MALFORMED_PACKET);
		cxa_stateMachine_transition(&mppIn->stateMachine, PROTO_STATE_WAIT_FIXEDHEADER_1);
		return;
	}
	cxa_mqtt_protocolParser_packetType_t packetType = (cxa_mqtt_protocolParser_packetType_t)(fixedHeader_1 >> 4);

	cxa_logger_trace(&mppIn->logger, "processing packet type %d", packetType);

	// now process it accordingly
	switch(packetType)
	{
		case CXA_MQTT_PACKETTYPE_CONNACK:
			processPacket_connAck(mppIn);
			break;

		case CXA_MQTT_PACKETTYPE_PINGRESP:
			processPacket_pingResp(mppIn);
			break;

		case CXA_MQTT_PACKETTYPE_SUBACK:
			processPacket_subAck(mppIn);
			break;

		case CXA_MQTT_PACKETTYPE_PUBLISH:
			processPacket_publish(mppIn);
			break;

		default:
			cxa_logger_warn(&mppIn->logger, "packet type not implemented: %d", packetType);
			break;
	}

	// nothing more to do here
	cxa_stateMachine_transition(&mppIn->stateMachine, PROTO_STATE_WAIT_FIXEDHEADER_1);
	return;
}


static void processPacket_connAck(cxa_mqtt_protocolParser_t *const mppIn)
{
	cxa_assert(mppIn);

	// ensure proper length
	if( cxa_fixedByteBuffer_getSize_bytes(&mppIn->fbb) != 4 )
	{
		cxa_logger_warn(&mppIn->logger, ERR_MALFORMED_PACKET);
		return;
	}

	// if we made it here, we have the right length...parse our fields
	uint8_t sessionPresent_raw;
	uint8_t retCode_raw;
	if( !cxa_fixedByteBuffer_get_uint8(&mppIn->fbb, 2, sessionPresent_raw) || !cxa_fixedByteBuffer_get_uint8(&mppIn->fbb, 3, retCode_raw) )
	{
		cxa_logger_warn(&mppIn->logger, ERR_MALFORMED_PACKET);
		return;
	}
	bool sessionPresent = sessionPresent_raw & 0x01;
	cxa_mqtt_protocolParser_connAck_returnCode_t retCode = retCode_raw;

	cxa_logger_trace(&mppIn->logger, "rx connack - sp:%d  rc:%d", sessionPresent, retCode);

	// notify our listeners
	cxa_array_iterate(&mppIn->listeners, currListener, cxa_mqtt_protocolParser_listenerEntry_t)
	{
		if( currListener == NULL ) return;
		if( currListener->cb_onConnAck != NULL ) currListener->cb_onConnAck(mppIn, sessionPresent, retCode, currListener->userVar);
	}
}


static void processPacket_pingResp(cxa_mqtt_protocolParser_t *const mppIn)
{
	cxa_assert(mppIn);

	// ensure proper length
	if( cxa_fixedByteBuffer_getSize_bytes(&mppIn->fbb) != 2 )
	{
		cxa_logger_warn(&mppIn->logger, ERR_MALFORMED_PACKET);
		return;
	}

	cxa_logger_trace(&mppIn->logger, "rx pingresp");

	// if we made it here, we have the right length...notify our listeners
	cxa_array_iterate(&mppIn->listeners, currListener, cxa_mqtt_protocolParser_listenerEntry_t)
	{
		if( currListener == NULL ) return;
		if( currListener->cb_onPingResp != NULL ) currListener->cb_onPingResp(mppIn, currListener->userVar);
	}
}


static void processPacket_subAck(cxa_mqtt_protocolParser_t *const mppIn)
{
	cxa_assert(mppIn);

	// ensure proper length (fixed so don't worry about variable length here)
	if( cxa_fixedByteBuffer_getSize_bytes(&mppIn->fbb) != 5 )
	{
		cxa_logger_warn(&mppIn->logger, ERR_MALFORMED_PACKET);
		return;
	}

	// get our fields
	uint16_t packetId;
	uint8_t retCode_raw;
	if( !cxa_fixedByteBuffer_get_uint16BE(&mppIn->fbb, 2, packetId) || !cxa_fixedByteBuffer_get_uint8(&mppIn->fbb, 4, retCode_raw) )
	{
		cxa_logger_warn(&mppIn->logger, ERR_MALFORMED_PACKET);
		return;
	}
	cxa_mqtt_protocolParser_subAck_returnCode_t retCode = retCode_raw;

	cxa_logger_trace(&mppIn->logger, "rx suback: pid:%d rc:%d", packetId, retCode);

	// notify our listeners
	cxa_array_iterate(&mppIn->listeners, currListener, cxa_mqtt_protocolParser_listenerEntry_t)
	{
		if( currListener == NULL ) return;
		if( currListener->cb_onSubAck != NULL ) currListener->cb_onSubAck(mppIn, packetId, retCode, currListener->userVar);
	}
}


static void processPacket_publish(cxa_mqtt_protocolParser_t *const mppIn)
{
	cxa_assert(mppIn);

	size_t currParseIndex = 0;

	uint8_t fixedHeader1;
	if( !cxa_fixedByteBuffer_get_uint8(&mppIn->fbb, currParseIndex, fixedHeader1) )
	{
		cxa_logger_warn(&mppIn->logger, ERR_MALFORMED_PACKET);
		return;
	}
	currParseIndex++;

	// get our fields from the fixed header
	bool dup = (fixedHeader1 >> 3) & 0x01;
	cxa_mqtt_protocolParser_qosLevel_t qos = (fixedHeader1 >> 1) & 0x03;
	bool retain = (fixedHeader1 & 0x01);

	// process our variable length
	bool isVarLengthComplete;
	size_t actualLength;
	size_t varLengthFieldLength_bytes;
	if( !processVariableLengthField(&mppIn->fbb, &isVarLengthComplete, &actualLength, &varLengthFieldLength_bytes) || !isVarLengthComplete )
	{
		cxa_logger_warn(&mppIn->logger, ERR_MALFORMED_PACKET);
		return;
	}
	currParseIndex += varLengthFieldLength_bytes;

	// topic name length first
	uint16_t topicNameLen_bytes;
	if( !cxa_fixedByteBuffer_get_uint16BE(&mppIn->fbb, currParseIndex, topicNameLen_bytes) )
	{
		cxa_logger_warn(&mppIn->logger, ERR_MALFORMED_PACKET);
		return;
	}
	currParseIndex += 2;

	// topic name
	char* topicName = (char*)cxa_fixedByteBuffer_get_pointerToIndex(&mppIn->fbb, currParseIndex);
	if( topicName == NULL )
	{
		cxa_logger_warn(&mppIn->logger, ERR_MALFORMED_PACKET);
		return;
	}
	currParseIndex += topicNameLen_bytes;

	// insert a null char to terminate the topic name
	if( !cxa_fixedByteBuffer_insert_uint8(&mppIn->fbb, currParseIndex, 0) )
	{
		cxa_logger_warn(&mppIn->logger, ERR_FBB_OVERFLOW);
		return;
	}
	currParseIndex++;

	// packetId iff qos 1 or 2
	uint16_t packetId = 0;
	if( qos != CXA_MQTT_QOS_ATMOST_ONCE )
	{
		if( !cxa_fixedByteBuffer_get_uint16BE(&mppIn->fbb, currParseIndex, packetId) )
		{
			cxa_logger_warn(&mppIn->logger, ERR_MALFORMED_PACKET);
			return;
		}
		currParseIndex += 2;
	}

	// double check our length
	size_t fbbLen_bytes = cxa_fixedByteBuffer_getSize_bytes(&mppIn->fbb);
	if( currParseIndex > fbbLen_bytes )
	{
		cxa_logger_warn(&mppIn->logger, ERR_MALFORMED_PACKET);
		return;
	}

	// payload and payload len
	size_t payloadLen_bytes = fbbLen_bytes - currParseIndex;
	void* payload = (payloadLen_bytes > 0) ? cxa_fixedByteBuffer_get_pointerToIndex(&mppIn->fbb, currParseIndex) : NULL;

	cxa_logger_trace(&mppIn->logger, "rx publish: '%s' %d bytes", topicName, payloadLen_bytes);

	// notify our listeners
	cxa_array_iterate(&mppIn->listeners, currListener, cxa_mqtt_protocolParser_listenerEntry_t)
	{
		if( currListener == NULL ) return;
		if( currListener->cb_onPublish != NULL ) currListener->cb_onPublish(mppIn, dup, qos, retain, topicName, packetId, payload, payloadLen_bytes, currListener->userVar);
	}
}


static bool processVariableLengthField(cxa_fixedByteBuffer_t *const fbbIn, bool *isCompleteOut, size_t *actualLengthOut, size_t *fieldLength_bytesOut)
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
	size_t multiplier = 1;
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
		if( multiplier > 128*128*128 )
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


static bool serialize_lengthAsVariableLengthEncoding(cxa_ioStream_t *iosIn, size_t packetLen_bytesIn)
{
	cxa_assert(iosIn);

	do
	{
		uint8_t currByte = packetLen_bytesIn % 128;
		packetLen_bytesIn = packetLen_bytesIn / 128;
		// if there are more data to encode, set the top bit of this byte
		if( packetLen_bytesIn > 0 ) packetLen_bytesIn |= 128;

		if( !cxa_ioStream_writeByte(iosIn, currByte) ) return false;
	} while(packetLen_bytesIn > 0);

	return true;
}


static bool serialize_cString(cxa_ioStream_t *iosIn, char *const stringIn)
{
	cxa_assert(iosIn);

	size_t strlen_bytes = (stringIn != NULL) ? strlen(stringIn) : 0;
	cxa_assert(strlen_bytes <= UINT16_MAX);

	if( !cxa_ioStream_writeByte(iosIn, ((strlen_bytes >> 8) & 0x00FF)) ) return false;
	if( !cxa_ioStream_writeByte(iosIn, ((strlen_bytes >> 0) & 0x00FF)) ) return false;

	return cxa_ioStream_writeBytes(iosIn, stringIn, strlen_bytes);
}
