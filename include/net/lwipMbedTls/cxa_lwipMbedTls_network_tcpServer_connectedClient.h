/**
 * @file
 * @copyright 2016 opencxa.org
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
#ifndef CXA_LWIPMBEDTLS_NETWORK_TCPSERVER_CONNECTEDCLIENT_H_
#define CXA_LWIPMBEDTLS_NETWORK_TCPSERVER_CONNECTEDCLIENT_H_


// ******** includes ********
#include <arpa/inet.h>
#include <lwip/sockets.h>

#include <cxa_network_tcpServer_connectedClient.h>
#include <cxa_timeDiff.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_lwipMbedTls_network_tcpServer_t object
 */
typedef struct cxa_lwipMbedTls_network_tcpServer_connectedClient cxa_lwipMbedTls_network_tcpServer_connectedClient_t;


/**
 * @private
 */
struct cxa_lwipMbedTls_network_tcpServer_connectedClient
{
	cxa_network_tcpServer_connectedClient_t super;

	int socket;
	char descriptiveString[23];			// "aaa.bbb.ccc.ddd::eeeee"

	cxa_timeDiff_t td_writeTimeout;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_lwipMbedTls_network_tcpServer_connectedClient_initUnbound(cxa_lwipMbedTls_network_tcpServer_connectedClient_t *const ccIn);


/**
 * @public
 */
void cxa_lwipMbedTls_network_tcpServer_connectedClient_bindToSocket(cxa_lwipMbedTls_network_tcpServer_connectedClient_t *const ccIn,
																	int socketIn,
																	struct sockaddr_in * clientAddressIn);


#endif // CXA_LWIPMBEDTLS_NETWORK_TCPSERVER_CONNECTEDCLIENT_H_
