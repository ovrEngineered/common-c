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
#include <cxa_led.h>
#include <cxa_mqtt_client_network.h>




// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
void cxa_mqtt_connManager_init(cxa_led_t *const ledConnIn,
							   char *const hostNameIn, uint16_t portNumIn, bool useTlsIn,
							   char *const usernameIn, uint8_t *const passwordIn, uint16_t passwordLen_bytesIn,
							   int threadIdIn);

void cxa_mqtt_connManager_init_clientCert(cxa_led_t *const ledConnIn,
										  char *const hostNameIn, uint16_t portNumIn,
										  const char* serverRootCertIn, size_t serverRootCertLen_bytesIn,
										  const char* clientCertIn, size_t clientCertLen_bytesIn,
										  const char* clientPrivateKeyIn, size_t clientPrivateKeyLen_bytesIn,
										  int threadIdIn);

cxa_mqtt_client_t* cxa_mqtt_connManager_getMqttClient(void);


#endif // CXA_MQTT_MAN_H_
