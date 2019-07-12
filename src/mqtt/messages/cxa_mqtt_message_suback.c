/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_mqtt_message_suback.h"


// ******** includes ********
#include <cxa_assert.h>

#define CXA_LOG_LEVEL				CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
bool cxa_mqtt_message_suback_getPacketId(cxa_mqtt_message_t *const msgIn, uint16_t *const packetIdOut)
{
	cxa_assert(msgIn);

	if( !msgIn->areFieldsConfigured || (cxa_mqtt_message_getType(msgIn) != CXA_MQTT_MSGTYPE_SUBACK) ) return false;

	uint16_t packetId_lcl;
	if( !cxa_linkedField_get_uint16BE(&msgIn->fields_suback.field_packetId, 0, packetId_lcl) ) return false;

	if( packetIdOut != NULL ) *packetIdOut = packetId_lcl;

	return true;
}


bool cxa_mqtt_message_suback_getReturnCode(cxa_mqtt_message_t *const msgIn, cxa_mqtt_subAck_returnCode_t *const returnCodeOut)
{
	cxa_assert(msgIn);

	if( !msgIn->areFieldsConfigured || (cxa_mqtt_message_getType(msgIn) != CXA_MQTT_MSGTYPE_SUBACK) ) return false;

	uint8_t returnCode_lcl;
	if( !cxa_linkedField_get_uint8(&msgIn->fields_suback.field_returnCode, 0, returnCode_lcl) ) return false;

	if( returnCodeOut != NULL ) *returnCodeOut = (cxa_mqtt_subAck_returnCode_t)returnCode_lcl;

	return true;
}


bool cxa_mqtt_message_suback_validateReceivedBytes(cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(msgIn);

	// packet id
	if( !cxa_linkedField_initChild_fixedLen(&msgIn->fields_suback.field_packetId, &msgIn->field_remainingLength, 2) ) return false;

	// return code
	if( !cxa_linkedField_initChild_fixedLen(&msgIn->fields_suback.field_returnCode, &msgIn->fields_suback.field_packetId, 1) ) return false;

	return true;
}


// ******** local function implementations ********
