/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_MQTT_MESSAGE_PUBLISH_H_
#define CXA_MQTT_MESSAGE_PUBLISH_H_


// ******** includes ********
#include <cxa_mqtt_message.h>


// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
bool cxa_mqtt_message_publish_init(cxa_mqtt_message_t *const msgIn, bool dupIn, cxa_mqtt_qosLevel_t qosIn, bool retainIn, char *const topicNameIn, uint16_t packedIdIn, void *const payloadIn, uint16_t payloadSize_bytesIn);

bool cxa_mqtt_message_publish_getTopicName(cxa_mqtt_message_t *const msgIn, char** topicNameOut, uint16_t *const topicNameLen_bytesOut);
bool cxa_mqtt_message_publish_getPayload(cxa_mqtt_message_t *const msgIn, cxa_linkedField_t **payloadLfOut);

bool cxa_mqtt_message_publish_topicName_trimToPointer(cxa_mqtt_message_t *const msgIn, char *const ptrIn);
bool cxa_mqtt_message_publish_topicName_prependCString(cxa_mqtt_message_t *const msgIn, char *const stringIn);
bool cxa_mqtt_message_publish_topicName_prependString_withLength(cxa_mqtt_message_t *const msgIn, char *const stringIn, size_t stringLen_bytesIn);
bool cxa_mqtt_message_publish_topicName_clear(cxa_mqtt_message_t *const msgIn);

/**
 * @protected
 */
bool cxa_mqtt_message_publish_validateReceivedBytes(cxa_mqtt_message_t *const msgIn);

#endif /* CXA_MQTT_MESSAGE_PUBLISH_H_ */
