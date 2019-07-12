/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_MQTT_MESSAGEFACTORY_H_
#define CXA_MQTT_MESSAGEFACTORY_H_


// ******** includes ********
#include <cxa_fixedByteBuffer.h>
#include <cxa_mqtt_message.h>
#include <cxa_config.h>


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
