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
#ifndef CXA_ESP8266_NETWORK_TCPSERVER_H_
#define CXA_ESP8266_NETWORK_TCPSERVER_H_


// ******** includes ********
#include <cxa_network_tcpServer.h>

#include <cxa_config.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_esp8266_network_tcpServer_t object
 */
typedef struct cxa_esp8266_network_tcpServer cxa_esp8266_network_tcpServer_t;


/**
 * @private
 */
struct cxa_esp8266_network_tcpServer
{
	cxa_network_tcpServer_t super;
};


// ******** global function prototypes ********
/**
 * @private
 */
void cxa_esp8266_network_tcpServer_init(cxa_esp8266_network_tcpServer_t *const tcpServerIn);


/**
 * @private
 */
void cxa_esp8266_network_tcpServer_update(cxa_esp8266_network_tcpServer_t *const tcpServerIn);


#endif // CXA_ESP8266_NETWORK_TCPSERVER_H_
