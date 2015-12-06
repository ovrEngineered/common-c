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
#ifndef CXA_MQTT_MESSAGE_CONNECT_H_
#define CXA_MQTT_MESSAGE_CONNECT_H_


// ******** includes ********
#include <cxa_mqtt_message.h>


// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
bool cxa_mqtt_message_connect_init(cxa_mqtt_message_t *const msgIn, char* clientIdIn, char* usernameIn, uint8_t* passwordIn, uint16_t passwordLen_bytesIn, bool cleanSessionIn, uint16_t keepAlive_sIn);

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
