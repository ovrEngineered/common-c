/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_MQTT_MESSAGE_SUBACK_H_
#define CXA_MQTT_MESSAGE_SUBACK_H_


// ******** includes ********
#include <cxa_mqtt_message.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef enum
{
	CXA_MQTT_SUBACK_RETCODE_SUCCESS_MAXQOS0=0,
	CXA_MQTT_SUBACK_RETCODE_SUCCESS_MAXQOS1=1,
	CXA_MQTT_SUBACK_RETCODE_SUCCESS_MAXQOS2=2,
	CXA_MQTT_SUBACK_RETCODE_FAILURE=128,
	CXA_MQTT_SUBACK_RETCODE_UNKNOWN=255,
}cxa_mqtt_subAck_returnCode_t;


// ******** global function prototypes ********
bool cxa_mqtt_message_suback_getPacketId(cxa_mqtt_message_t *const msgIn, uint16_t *const packetIdOut);
bool cxa_mqtt_message_suback_getReturnCode(cxa_mqtt_message_t *const msgIn, cxa_mqtt_subAck_returnCode_t *const returnCodeOut);


/**
 * @protected
 */
bool cxa_mqtt_message_suback_validateReceivedBytes(cxa_mqtt_message_t *const msgIn);

#endif /* CXA_MQTT_MESSAGE_SUBACK_H_ */
