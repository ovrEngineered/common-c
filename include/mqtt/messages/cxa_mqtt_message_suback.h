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
#ifndef CXA_MQTT_MESSAGE_SUBACK_H_
#define CXA_MQTT_MESSAGE_SUBACK_H_


// ******** includes ********
#include <cxa_mqtt_message.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef enum
{
	CXA_MQTT_SUBACK_RETCODE_SUCCESS_MAXQOS0=0,
	CXA_MQTT_SUBACK_RETCODE_SUCCESS_MAXQOS1=1,
	CXA_MQTT_SUBACK_RETCODE_SUCCESS_MAXQOS2=2,
	CXA_MQTT_SUBACK_RETCODE_FAILURE=128,
	CXA_MQTT_SUBACK_RETCODE_UNKNOWN=255,
}cxa_mqtt_subAck_returnCode_t;


// ******** global function prototypes ********
bool cxa_mqtt_message_suback_getPacketId(cxa_mqtt_message_t *const msgIn, uint16_t *const packetIdOut);
bool cxa_mqtt_message_suback_getReturnCode(cxa_mqtt_message_t *const msgIn, cxa_mqtt_subAck_returnCode_t *const returnCodeOut);


/**
 * @protected
 */
bool cxa_mqtt_message_suback_validateReceivedBytes(cxa_mqtt_message_t *const msgIn);

#endif /* CXA_MQTT_MESSAGE_SUBACK_H_ */
