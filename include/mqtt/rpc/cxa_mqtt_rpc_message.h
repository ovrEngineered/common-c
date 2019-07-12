/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_MQTT_RPC_MESSAGE_H_
#define CXA_MQTT_RPC_MESSAGE_H_


// ******** includes ********
#include <stdbool.h>
#include <cxa_mqtt_message.h>


// ******** global macro definitions ********
#define CXA_MQTT_RPC_MESSAGE_VERSION				"v1"


// ******** global type definitions *********


// ******** global function prototypes ********
bool cxa_mqtt_rpc_message_isActionableResponse(cxa_mqtt_message_t *const msgIn, char** methodNameOut, size_t* methodNameLen_bytesOut, char** idOut, size_t* idLen_bytesOut);

#endif /* CXA_MQTT_RPC_MESSAGE_H_ */
