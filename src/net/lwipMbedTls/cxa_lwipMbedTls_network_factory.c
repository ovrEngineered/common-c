/**
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
#include "cxa_network_factory.h"


// ******** includes ********
#include <stdbool.h>

#include <cxa_lwipMbedTls_network_tcpClient.h>

#include <cxa_config.h>


// ******** local macro definitions ********
#ifndef CXA_LWIPMBEDTLS_MAXNUM_TCP_CLIENTS
	#define CXA_LWIPMBEDTLS_MAXNUM_TCP_CLIENTS		1
#endif

#ifndef CXA_LWIPMBDEDTLS_MAXNUM_TCP_SERVERS
	#define CXA_LWIPMBDEDTLS_MAXNUM_TCP_SERVERS	0
#else
	#error "TCP server support not available"
#endif


// ******** local type definitions ********
typedef struct
{
	cxa_lwipMbedTls_network_tcpClient_t client;
	bool isReserved;
}tcpClient_entry_t;


// ******** local function prototypes ********
static void cxa_network_factory_init(void);


// ********  local variable declarations *********
static bool isInit = false;

#if CXA_LWIPMBEDTLS_MAXNUM_TCP_CLIENTS > 0
static tcpClient_entry_t tcpClientMap[CXA_LWIPMBEDTLS_MAXNUM_TCP_CLIENTS];
#endif


// ******** global function implementations ********
cxa_network_tcpClient_t* cxa_network_factory_reserveTcpClient(void)
{
	if( !isInit ) cxa_network_factory_init();

	cxa_network_tcpClient_t* retVal = NULL;

	for( size_t i = 0; i < (sizeof(tcpClientMap)/sizeof(*tcpClientMap)); i++ )
	{
		if( !tcpClientMap[i].isReserved )
		{
			tcpClientMap[i].isReserved = true;
			retVal = &tcpClientMap[i].client.super;
			break;
		}
	}

	return retVal;
}


void cxa_network_factory_freeTcpClient(cxa_network_tcpClient_t *const clientIn)
{
	for( size_t i = 0; i < (sizeof(tcpClientMap)/sizeof(*tcpClientMap)); i++ )
	{
		if( &tcpClientMap[i].client.super == clientIn )
		{
			tcpClientMap[i].isReserved = false;
			break;
		}
	}
}


cxa_network_tcpServer_t* cxa_network_factory_reserveTcpServer(void)
{
	return NULL;
}


void cxa_network_factory_freeTcpServer(cxa_network_tcpServer_t *const serverIn)
{

}


// ******** local function implementations ********
static void cxa_network_factory_init(void)
{
#if CXA_LWIPMBEDTLS_MAXNUM_TCP_CLIENTS > 0
	for( size_t i = 0; i < (sizeof(tcpClientMap)/sizeof(*tcpClientMap)); i++ )
	{
		tcpClientMap[i].isReserved = false;
		cxa_lwipMbedTls_network_tcpClient_init(&tcpClientMap[i].client);
	}
#endif

	isInit = true;
}
