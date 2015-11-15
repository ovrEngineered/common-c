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
#ifndef CXA_MQTT_PROTOCOLPARSER_H_
#define CXA_MQTT_PROTOCOLPARSER_H_


// ******** includes ********
#include <cxa_array.h>
#include <cxa_fixedByteBuffer.h>
#include <cxa_ioStream.h>
#include <cxa_logger_header.h>
#include <cxa_stateMachine.h>
#include <cxa_config.h>


// ******** global macro definitions ********
#ifndef CXA_MQTT_PROTOCOLPARSER_MAXNUM_LISTENERS
	#define CXA_MQTT_PROTOCOLPARSER_MAXNUM_LISTENERS			1
#endif

#ifndef CXA_MQTT_PROTOCOLPARSER_RXBUFFER_SIZE_BYTES
	#define CXA_MQTT_PROTOCOLPARSER_RXBUFFER_SIZE_BYTES			64
#endif


// ******** global type definitions *********
typedef struct cxa_mqtt_protocolParser cxa_mqtt_protocolParser_t;


typedef enum
{
	CXA_MQTT_PACKETTYPE_CONNECT=1,
	CXA_MQTT_PACKETTYPE_CONNACK=2,
	CXA_MQTT_PACKETTYPE_PUBLISH=3,
	CXA_MQTT_PACKETTYPE_SUBSCRIBE=8,
	CXA_MQTT_PACKETTYPE_SUBACK=9,
	CXA_MQTT_PACKETTYPE_PINGREQ=12,
	CXA_MQTT_PACKETTYPE_PINGRESP=13
}cxa_mqtt_protocolParser_packetType_t;


typedef enum
{
	CXA_MQTT_CONNACK_RETCODE_ACCEPTED,
	CXA_MQTT_CONNACK_RETCODE_REFUSED_PROTO,
	CXA_MQTT_CONNACK_RETCODE_REFUSED_CID,
	CXA_MQTT_CONNACK_RETCODE_REFUSED_SERVERUNAVAILABLE,
	CXA_MQTT_CONNACK_RETCODE_REFUSED_BADUSERNAMEPASSWORD,
	CXA_MQTT_CONNACK_RETCODE_REFUSED_NOTAUTHORIZED
}cxa_mqtt_protocolParser_connAck_returnCode_t;


typedef enum
{
	CXA_MQTT_SUBACK_RETCODE_SUCCESS_MAXQOS0,
	CXA_MQTT_SUBACK_RETCODE_SUCCESS_MAXQOS1,
	CXA_MQTT_SUBACK_RETCODE_SUCCESS_MAXQOS2,
	CXA_MQTT_SUBACK_RETCODE_FAILURE
}cxa_mqtt_protocolParser_subAck_returnCode_t;


typedef enum
{
	CXA_MQTT_QOS_ATMOST_ONCE=0,
	//CXA_MQTT_QOS_ATLEAST_ONCE=1, -- not supported
	//CXA_MQTT_QOS_EXACTLY_ONCE -- not supported
}cxa_mqtt_protocolParser_qosLevel_t;


typedef void (*cxa_mqtt_protocolParser_cb_onConnAck_t)(cxa_mqtt_protocolParser_t *const mppIn, bool sessionPresentIn, cxa_mqtt_protocolParser_connAck_returnCode_t retCodeIn, void *const userVarIn);
typedef void (*cxa_mqtt_protocolParser_cb_onPingResp_t)(cxa_mqtt_protocolParser_t *const mppIn, void *const userVarIn);
typedef void (*cxa_mqtt_protocolParser_cb_onSubAck_t)(cxa_mqtt_protocolParser_t *const mppIn, uint16_t packetIdIn, cxa_mqtt_protocolParser_subAck_returnCode_t retCodeIn, void *const userVarIn);
typedef void (*cxa_mqtt_protocolParser_cb_onPublish_t)(cxa_mqtt_protocolParser_t *const mppIn,
				bool dupIn, cxa_mqtt_protocolParser_qosLevel_t qosIn, bool retainIn, char* topicNameIn, uint16_t packetIdIn,
				void* payloadIn, size_t payloadLen_bytesIn, void *const userVarIn);


typedef struct
{
	cxa_mqtt_protocolParser_cb_onConnAck_t cb_onConnAck;
	cxa_mqtt_protocolParser_cb_onPingResp_t cb_onPingResp;
	cxa_mqtt_protocolParser_cb_onSubAck_t cb_onSubAck;
	cxa_mqtt_protocolParser_cb_onPublish_t cb_onPublish;

	void *userVar;
}cxa_mqtt_protocolParser_listenerEntry_t;


struct cxa_mqtt_protocolParser
{
	cxa_ioStream_t *ioStream;

	cxa_array_t listeners;
	cxa_mqtt_protocolParser_listenerEntry_t listeners_raw[CXA_MQTT_PROTOCOLPARSER_MAXNUM_LISTENERS];

	cxa_fixedByteBuffer_t fbb;
	uint8_t fbb_raw[CXA_MQTT_PROTOCOLPARSER_RXBUFFER_SIZE_BYTES];
	size_t remainingBytesToReceive;

	cxa_stateMachine_t stateMachine;
	cxa_logger_t logger;
};


// ******** global function prototypes ********
void cxa_mqtt_protocolParser_init(cxa_mqtt_protocolParser_t *const mppIn, cxa_ioStream_t *const ioStreamIn);

void cxa_mqtt_protocolParser_addListener(cxa_mqtt_protocolParser_t *const mppIn,
										 cxa_mqtt_protocolParser_cb_onConnAck_t cb_onConnAckIn,
										 cxa_mqtt_protocolParser_cb_onPingResp_t cb_onPingResp,
										 cxa_mqtt_protocolParser_cb_onSubAck_t cb_onSubAckIn,
										 cxa_mqtt_protocolParser_cb_onPublish_t cb_onPublishIn,
										 void *const userVarIn);

bool cxa_mqtt_protocolParser_writePacket_connect(cxa_mqtt_protocolParser_t *const mppIn, char* clientIdIn, char* usernameIn, char* passwordIn, bool cleanSessionIn, uint16_t keepAlive_sIn);
bool cxa_mqtt_protocolParser_writePacket_pingReq(cxa_mqtt_protocolParser_t *const mppIn);
bool cxa_mqtt_protocolParser_writePacket_publish(cxa_mqtt_protocolParser_t *const mppIn, cxa_mqtt_protocolParser_qosLevel_t qosIn, bool retainIn,
												 char* topicNameIn, void *const payloadIn, size_t payloadLen_bytesIn);
bool cxa_mqtt_protocolParser_writePacket_subscribe(cxa_mqtt_protocolParser_t *const mppIn, uint16_t packetIdIn, char *topicFilterIn, cxa_mqtt_protocolParser_qosLevel_t qosIn);


void cxa_mqtt_protocolParser_update(cxa_mqtt_protocolParser_t *const mppIn);


char* cxa_mqtt_protocolParser_getStringForConnAckRetCode(cxa_mqtt_protocolParser_connAck_returnCode_t retCodeIn);


#endif // CXA_MQTT_PROTOCOLPARSER_H_
