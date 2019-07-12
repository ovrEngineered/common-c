/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_mqtt_message_connack.h"


// ******** includes ********
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
bool cxa_mqtt_message_connack_init(cxa_mqtt_message_t *const msgIn, bool isSessionPresentIn, cxa_mqtt_connAck_returnCode_t retCodeIn)
{
	cxa_assert(msgIn);

	// fixed header 1
	if( !cxa_linkedField_initRoot_fixedLen(&msgIn->field_packetTypeAndFlags, msgIn->buffer, 0, 1) ||
			!cxa_linkedField_append_uint8(&msgIn->field_packetTypeAndFlags, (CXA_MQTT_MSGTYPE_CONNACK << 4) ) ) return false;

	// remaining length
	if( !cxa_linkedField_initChild(&msgIn->field_remainingLength, &msgIn->field_packetTypeAndFlags, 0) ) return false;

	// session present (connect ack flags)
	if( !cxa_linkedField_initChild_fixedLen(&msgIn->fields_connack.field_sessionPresent, &msgIn->field_remainingLength, 1) ||
			!cxa_linkedField_append_uint8(&msgIn->fields_connack.field_sessionPresent, isSessionPresentIn) ) return false;

	// connect return code
	if( !cxa_linkedField_initChild_fixedLen(&msgIn->fields_connack.field_returnCode, &msgIn->fields_connack.field_sessionPresent, 1) ||
				!cxa_linkedField_append_uint8(&msgIn->fields_connack.field_returnCode, retCodeIn) ) return false;

	msgIn->areFieldsConfigured = true;
	return true;
}


bool cxa_mqtt_message_connack_isSessionPresent(cxa_mqtt_message_t *const msgIn, bool *const isSessionPresentOut)
{
	cxa_assert(msgIn);

	if( !msgIn->areFieldsConfigured || (cxa_mqtt_message_getType(msgIn) != CXA_MQTT_MSGTYPE_CONNACK) ) return false;

	uint8_t sessionPresent_lcl;
	if( !cxa_linkedField_get_uint8(&msgIn->fields_connack.field_sessionPresent, 0, sessionPresent_lcl) ) return false;

	if( isSessionPresentOut != NULL ) *isSessionPresentOut = sessionPresent_lcl & 0x01;

	return true;
}


bool cxa_mqtt_message_connack_getReturnCode(cxa_mqtt_message_t *const msgIn, cxa_mqtt_connAck_returnCode_t *const returnCodeOut)
{
	cxa_assert(msgIn);

	if( !msgIn->areFieldsConfigured || (cxa_mqtt_message_getType(msgIn) != CXA_MQTT_MSGTYPE_CONNACK) ) return false;

	uint8_t returnCode_lcl;
	if( !cxa_linkedField_get_uint8(&msgIn->fields_connack.field_returnCode, 0, returnCode_lcl) ) return false;

	if( returnCodeOut != NULL ) *returnCodeOut = (cxa_mqtt_connAck_returnCode_t)returnCode_lcl;

	return true;
}


bool cxa_mqtt_message_connack_validateReceivedBytes(cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(msgIn);

	// session present
	if( !cxa_linkedField_initChild_fixedLen(&msgIn->fields_connack.field_sessionPresent, &msgIn->field_remainingLength, 1) ) return false;

	// return code
	if( !cxa_linkedField_initChild_fixedLen(&msgIn->fields_connack.field_returnCode, &msgIn->fields_connack.field_sessionPresent, 1) ) return false;

	return true;
}


// ******** local function implementations ********
