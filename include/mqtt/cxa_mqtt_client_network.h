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
#ifndef CXA_MQTT_CLIENT_NETWORK_H_
#define CXA_MQTT_CLIENT_NETWORKH_


// ******** includes ********
#include <cxa_mqtt_client.h>
#include <cxa_network_tcpClient.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct cxa_mqtt_client_network cxa_mqtt_client_network_t;


/**
 * @private
 */
struct cxa_mqtt_client_network
{
	cxa_mqtt_client_t super;

	cxa_network_tcpClient_t *netClient;

	char* username;
	uint8_t* password;
	uint16_t passwordLen_bytes;
};


// ******** global function prototypes ********
void cxa_mqtt_client_network_init(cxa_mqtt_client_network_t *const clientIn, char *const clientIdIn);

bool cxa_mqtt_client_network_connectToHost(cxa_mqtt_client_network_t *const clientIn, char *const hostNameIn, uint16_t portNumIn, bool useTlsIn,
										   char *const usernameIn, uint8_t *const passwordIn, uint16_t passwordLen_bytesIn);

bool cxa_mqtt_client_network_connectToHost_clientCert(cxa_mqtt_client_network_t *const clientIn, char *const hostNameIn, uint16_t portNumIn,
														const char* serverRootCertIn, size_t serverRootCertLen_bytesIn,
														const char* clientCertIn, size_t clientCertLen_bytesIn,
														const char* clientPrivateKeyIn, size_t clientPrivateKeyLen_bytesIn);

#endif // CXA_MQTT_CLIENT_H_
