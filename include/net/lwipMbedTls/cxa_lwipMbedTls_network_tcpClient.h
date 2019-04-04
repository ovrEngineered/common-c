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
#ifndef CXA_LWIPMBEDTLS_NETWORK_TCPCLIENT_H_
#define CXA_LWIPMBEDTLS_NETWORK_TCPCLIENT_H_


// ******** includes ********
#include <cxa_network_tcpClient.h>

#include <cxa_stateMachine.h>
#include <cxa_timeDiff.h>

#include <mbedtls/net.h>
#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/error.h>
#include <mbedtls/certs.h>


// ******** global macro definitions ********
#ifndef CXA_LWIPMBEDTLS_NETWORK_TCPCLIENT_MAXHOSTNAMELEN_BYTES
	#define CXA_LWIPMBEDTLS_NETWORK_TCPCLIENT_MAXHOSTNAMELEN_BYTES			64
#endif

#ifndef CXA_LWIPMBEDTLS_NETWORK_TCPCLIENT_MAXPORTNUMLEN_BYTES
	#define CXA_LWIPMBEDTLS_NETWORK_TCPCLIENT_MAXPORTNUMLEN_BYTES			5
#endif


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_lwipMbedTls_network_tcpClient_t object
 */
typedef struct cxa_lwipMbedTls_network_tcpClient cxa_lwipMbedTls_network_tcpClient_t;


/**
 * @private
 */
struct cxa_lwipMbedTls_network_tcpClient
{
	cxa_network_tcpClient_t super;

	char targetHostName[CXA_LWIPMBEDTLS_NETWORK_TCPCLIENT_MAXHOSTNAMELEN_BYTES+1];
//	ip_addr_t targetIp;
	char targetPortNum[CXA_LWIPMBEDTLS_NETWORK_TCPCLIENT_MAXPORTNUMLEN_BYTES+1];

	cxa_timeDiff_t td_writeTimeout;
	cxa_stateMachine_t stateMachine;

	bool useClientCert;
	struct
	{
	    mbedtls_entropy_context entropy;
	    mbedtls_ctr_drbg_context ctr_drbg;
	    mbedtls_ssl_context sslContext;
	    mbedtls_x509_crt cert_server;
	    mbedtls_x509_crt cert_client;
	    mbedtls_pk_context client_key_private;
	    mbedtls_ssl_config conf;
	    mbedtls_net_context server_fd;

	    struct
		{
			bool areBasicsInitialized;

			uint16_t crc_serverRootCert;
			uint16_t crc_clientCert;
			uint16_t crc_clientPrivateKey;
		}initState;
	}tls;
};


// ******** global function prototypes ********
/**
 * @private
 */
void cxa_lwipMbedTls_network_tcpClient_init(cxa_lwipMbedTls_network_tcpClient_t *const netClientIn, int threadIdIn);


#endif // CXA_LWIPMBEDTLS_NETWORK_TCPCLIENT_H_
