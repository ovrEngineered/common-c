/**
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
#include "cxa_mqtt_message_connack.h"


// ******** includes ********
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
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
