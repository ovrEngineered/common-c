/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_PROTOCOLPARSER_MQTT_H_
#define CXA_PROTOCOLPARSER_MQTT_H_


// ******** includes ********
#include <cxa_protocolParser.h>
#include <cxa_mqtt_message.h>
#include <cxa_stateMachine.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct
{
	cxa_protocolParser_t super;

	cxa_stateMachine_t stateMachine;
	size_t remainingBytesToReceive;
}cxa_protocolParser_mqtt_t;


// ******** global function prototypes ********
void cxa_protocolParser_mqtt_init(cxa_protocolParser_mqtt_t *const mppIn, cxa_ioStream_t *const ioStreamIn, cxa_fixedByteBuffer_t *const buffIn, int threadIdIn);


#endif // CXA_PROTOCOLPARSER_MQTT_H_
