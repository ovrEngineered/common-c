/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_MQTT_MESSAGE_CONNECT_H_
#define CXA_MQTT_MESSAGE_CONNECT_H_


// ******** includes ********
#include <cxa_mqtt_message.h>


// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
bool cxa_mqtt_message_connect_init(cxa_mqtt_message_t *const msgIn, char* clientIdIn,
								   char* usernameIn, uint8_t* passwordIn, uint16_t passwordLen_bytesIn,
								   cxa_mqtt_qosLevel_t willQosIn, bool willRetainIn, const char* willTopicIn, void *const willPayloadIn, size_t willPayloadLen_bytesIn,
								   bool cleanSessionIn, uint16_t keepAlive_sIn);

bool cxa_mqtt_message_connect_hasWill(cxa_mqtt_message_t *const msgIn, bool *const hasWillOut);
bool cxa_mqtt_message_connect_hasUsername(cxa_mqtt_message_t *const msgIn, bool *const hasUsernameOut);
bool cxa_mqtt_message_connect_hasPassword(cxa_mqtt_message_t *const msgIn, bool *const hasPasswordOut);
bool cxa_mqtt_message_connect_cleanSessionRequested(cxa_mqtt_message_t *const msgIn, bool *const cleanSessionRequestedOut);

bool cxa_mqtt_message_connect_getClientId(cxa_mqtt_message_t *const msgIn, char** clientIdOut, uint16_t* clientIdLen_bytesOut);
bool cxa_mqtt_message_connect_getUsername(cxa_mqtt_message_t *const msgIn, char** usernameOut, uint16_t* usernameLen_bytesOut);
bool cxa_mqtt_message_connect_getPassword(cxa_mqtt_message_t *const msgIn, uint8_t** passwordOut, uint16_t* passwordLen_bytesOut);

/**
 * @protected
 */
bool cxa_mqtt_message_connect_validateReceivedBytes(cxa_mqtt_message_t *const msgIn);

#endif /* CXA_MQTT_MESSAGE_CONNECT_H_ */
