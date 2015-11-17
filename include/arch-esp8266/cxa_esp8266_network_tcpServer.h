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
#include <cxa_fixedFifo.h>
#include <cxa_stateMachine.h>

#include <c_types.h>
#include <ip_addr.h>
#include <espconn.h>
#include <cxa_config.h>


// ******** global macro definitions ********
#ifndef CXA_ESP8266_NETWORK_TCPSERVER_RXBUFFERSIZE_BYTES
	#define CXA_ESP8266_NETWORK_TCPSERVER_RXBUFFERSIZE_BYTES			64
#endif

#ifndef CXA_ESP8266_NETWORK_TCPSERVER_TXBUFFERSIZE_BYTES
	#define CXA_ESP8266_NETWORK_TCPSERVER_TXBUFFERSIZE_BYTES			64
#endif


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

	ip_addr_t ip;
	struct espconn espconn_listen;
	esp_tcp tcp;
	struct espconn* espconn_client;

	cxa_stateMachine_t stateMachine;

	cxa_fixedFifo_t rxFifo;
	uint8_t rxFifo_raw[CXA_ESP8266_NETWORK_TCPSERVER_RXBUFFERSIZE_BYTES];

	cxa_fixedFifo_t txFifo;
	uint8_t txFifo_raw[CXA_ESP8266_NETWORK_TCPSERVER_TXBUFFERSIZE_BYTES];
	size_t numBytesInPreviousBulkDequeue;
	bool sendInProgress;
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
