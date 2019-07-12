/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_mqtt_message_pingRequest.h"


// ******** includes ********
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
bool cxa_mqtt_message_pingRequest_init(cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(msgIn);

	// fixed header 1
	if( !cxa_linkedField_initRoot_fixedLen(&msgIn->field_packetTypeAndFlags, msgIn->buffer, 0, 1) ||
			!cxa_linkedField_append_uint8(&msgIn->field_packetTypeAndFlags, (CXA_MQTT_MSGTYPE_PINGREQ << 4) ) ) return false;

	// remaining length
	if( !cxa_linkedField_initChild(&msgIn->field_remainingLength, &msgIn->field_packetTypeAndFlags, 0) ) return false;

	msgIn->areFieldsConfigured = true;
	return true;
}


bool cxa_mqtt_message_pingRequest_validateReceivedBytes(cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(msgIn);

	// @TODO should probably make sure there aren't any following bytes (per spec)

	return true;
}


// ******** local function implementations ********
