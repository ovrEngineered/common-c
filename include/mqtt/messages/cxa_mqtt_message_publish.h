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
#ifndef CXA_MQTT_MESSAGE_PUBLISH_H_
#define CXA_MQTT_MESSAGE_PUBLISH_H_


// ******** includes ********
#include <cxa_mqtt_message.h>


// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
bool cxa_mqtt_message_publish_init(cxa_mqtt_message_t *const msgIn, bool dupIn, cxa_mqtt_qosLevel_t qosIn, bool retainIn, char *const topicNameIn, uint16_t packedIdIn, void *const payloadIn, uint16_t payloadSize_bytesIn);


bool cxa_mqtt_message_publish_getTopicName(cxa_mqtt_message_t *const msgIn, char** topicNameOut, uint16_t *const topicNameLen_bytesOut);
bool cxa_mqtt_message_publish_getPayload(cxa_mqtt_message_t *const msgIn, void** payloadOut, size_t *const payloadSize_bytesOut);

bool cxa_mqtt_message_publish_topicName_trimToPointer(cxa_mqtt_message_t *const msgIn, char *const ptrIn);
bool cxa_mqtt_message_publish_topicName_prependCString(cxa_mqtt_message_t *const msgIn, char *const stringIn);

/**
 * @protected
 */
bool cxa_mqtt_message_publish_validateReceivedBytes(cxa_mqtt_message_t *const msgIn);

#endif /* CXA_MQTT_MESSAGE_PUBLISH_H_ */
