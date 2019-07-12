/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_MQTT_MESSAGE_CONNACK_H_
#define CXA_MQTT_MESSAGE_CONNACK_H_


// ******** includes ********
#include <cxa_mqtt_message.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef enum
{
	CXA_MQTT_CONNACK_RETCODE_ACCEPTED=0,
	CXA_MQTT_CONNACK_RETCODE_REFUSED_PROTO=1,
	CXA_MQTT_CONNACK_RETCODE_REFUSED_CID=2,
	CXA_MQTT_CONNACK_RETCODE_REFUSED_SERVERUNAVAILABLE=3,
	CXA_MQTT_CONNACK_RETCODE_REFUSED_BADUSERNAMEPASSWORD=4,
	CXA_MQTT_CONNACK_RETCODE_REFUSED_NOTAUTHORIZED=5,
	CXA_MQTT_CONNACK_RETCODE_UNKNOWN=255
}cxa_mqtt_connAck_returnCode_t;


// ******** global function prototypes ********
bool cxa_mqtt_message_connack_init(cxa_mqtt_message_t *const msgIn, bool isSessionPresentIn, cxa_mqtt_connAck_returnCode_t retCodeIn);

bool cxa_mqtt_message_connack_isSessionPresent(cxa_mqtt_message_t *const msgIn, bool *const isSessionPresentOut);
bool cxa_mqtt_message_connack_getReturnCode(cxa_mqtt_message_t *const msgIn, cxa_mqtt_connAck_returnCode_t *const returnCodeOut);


/**
 * @protected
 */
bool cxa_mqtt_message_connack_validateReceivedBytes(cxa_mqtt_message_t *const msgIn);

#endif /* CXA_MQTT_MESSAGE_CONNACK_H_ */
