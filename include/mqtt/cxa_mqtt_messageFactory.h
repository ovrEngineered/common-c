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
#ifndef CXA_MQTT_MESSAGEFACTORY_H_
#define CXA_MQTT_MESSAGEFACTORY_H_


// ******** includes ********
#include <cxa_fixedByteBuffer.h>
#include <cxa_mqtt_message.h>


// ******** global macro definitions ********
#ifndef CXA_MQTT_MESSAGEFACTORY_NUM_MESSAGES
	#define CXA_MQTT_MESSAGEFACTORY_NUM_MESSAGES			2
#endif

#ifndef CXA_MQTT_MESSAGEFACTORY_MESSAGE_SIZE_BYTES
	#define CXA_MQTT_MESSAGEFACTORY_MESSAGE_SIZE_BYTES		64
#endif


// ******** global type definitions *********


// ******** global function prototypes ********
size_t cxa_mqtt_messageFactory_getNumFreeMessages(void);
cxa_mqtt_message_t* cxa_mqtt_messageFactory_getFreeMessage_empty(void);

cxa_mqtt_message_t* cxa_mqtt_messageFactory_getMessage_byBuffer(cxa_fixedByteBuffer_t *const fbbIn);

void cxa_mqtt_messageFactory_incrementMessageRefCount(cxa_mqtt_message_t *const msgIn);
void cxa_mqtt_messageFactory_decrementMessageRefCount(cxa_mqtt_message_t *const msgIn);
uint8_t cxa_mqtt_messageFactory_getReferenceCountForMessage(cxa_mqtt_message_t *const msgIn);


#endif /* CXA_MQTT_MESSAGEFACTORY_H_ */
