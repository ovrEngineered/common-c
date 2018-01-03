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
#ifndef CXA_MQTT_CONN_MAN_H_
#define CXA_MQTT_CONN_MAN_H_


// ******** includes ********
#include <cxa_mqtt_client_network.h>




// ******** global macro definitions ********


// ******** global type definitions *********
typedef void (*cxa_mqtt_connManager_enteringStandoffCb_t)(void *const userVarIn);
typedef bool (*cxa_mqtt_connManager_canLeaveStandoffCb_t)(void *const userVarIn);


// ******** global function prototypes ********
void cxa_mqtt_connManager_init(char *const hostNameIn, uint16_t portNumIn, int threadIdIn);

void cxa_mqtt_connManager_setStandoffManagementCbs(cxa_mqtt_connManager_enteringStandoffCb_t cb_enteringStandoffIn,
												  cxa_mqtt_connManager_canLeaveStandoffCb_t cb_canLeaveStandoffCbIn,
												  void *const userVarIn);

bool cxa_mqtt_connManager_areCredentialsSet(void);

bool cxa_mqtt_connManager_start(void);
void cxa_mqtt_connManager_stop(void);

uint32_t cxa_mqtt_connManager_getNumFailedConnects(void);

cxa_mqtt_client_t* cxa_mqtt_connManager_getMqttClient(void);


#endif // CXA_MQTT_MAN_H_
