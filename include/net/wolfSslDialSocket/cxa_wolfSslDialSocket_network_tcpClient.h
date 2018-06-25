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
#ifndef CXA_WOLFSSLDIALSOCKET_NETWORK_TCPCLIENT_H_
#define CXA_WOLFSSLDIALSOCKET_NETWORK_TCPCLIENT_H_


// ******** includes ********
#include <aq_telitTsvgModem.h>
#include <cxa_network_tcpClient.h>
#include <cxa_stateMachine.h>
#include <cxa_timeDiff.h>

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>


// ******** global macro definitions ********
#ifndef CXA_WOLFSSLDIALSOCKET_NETWORK_TCPCLIENT_MAXHOSTNAMELEN_BYTES
	#define CXA_WOLFSSLDIALSOCKET_NETWORK_TCPCLIENT_MAXHOSTNAMELEN_BYTES			64
#endif

#ifndef CXA_WOLFSSLDIALSOCKET_NETWORK_TCPCLIENT_MAXPORTNUMLEN_BYTES
	#define CXA_WOLFSSLDIALSOCKET_NETWORK_TCPCLIENT_MAXPORTNUMLEN_BYTES			5
#endif


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_wolfSslDialSocket_network_tcpClient_t object
 */
typedef struct cxa_wolfSslDialSocket_network_tcpClient cxa_wolfSslDialSocket_network_tcpClient_t;


/**
 * @private
 */
struct cxa_wolfSslDialSocket_network_tcpClient
{
	cxa_network_tcpClient_t super;
    
    aq_telitTsvgModem_t* modem;
    cxa_ioStream_t* modemIoStream;

	char targetHostName[CXA_WOLFSSLDIALSOCKET_NETWORK_TCPCLIENT_MAXHOSTNAMELEN_BYTES+1];
	uint16_t targetPortNum;
    
	cxa_stateMachine_t stateMachine;

	bool useClientCert;
	struct
	{
        WOLFSSL_CTX* ctx;
        WOLFSSL*     ssl;

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
void cxa_wolfSslDialSocket_network_tcpClient_init(cxa_wolfSslDialSocket_network_tcpClient_t *const netClientIn,
                                                    aq_telitTsvgModem_t *const modemIn,
                                                    int threadIdIn);


#endif // CXA_WOLFSSLDIALSOCKET_NETWORK_TCPCLIENT_H_
