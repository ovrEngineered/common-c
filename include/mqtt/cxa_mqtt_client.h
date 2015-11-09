/**
 * @file
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
#ifndef CXA_ESP8266_MQTT_CLIENT_H_
#define CXA_ESP8266_MQTT_CLIENT_H_


// ******** includes ********
#include <stdbool.h>
#include <MQTTPacket.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_esp8266_mqttClient_t object
 */
typedef struct cxa_esp8266_mqttClient cxa_esp8266_mqttClient_t;


/**
 * @private
 */
struct cxa_esp8266_mqttClient
{
	MQTTTransport transport;
};


// ******** global function prototypes ********
void cxa_esp8266_mqttClient_init(cxa_esp8266_mqttClient_t *const clientIn);

bool cxa_esp8266_mqttClient_connectTohost(cxa_esp8266_mqttClient_t *const clientIn, char *const hostIn, uint16_t portNumIn);


#endif // CXA_ESP8266_MQTT_CLIENT_H_
