/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_wolfSslDialSocket_network_factory.h"


// ******** includes ********
#include <stdbool.h>

#include <cxa_assert.h>
#include <cxa_config.h>
#include <cxa_wolfSslDialSocket_network_tcpClient.h>


// ******** local macro definitions ********
#ifndef CXA_WOLFSSLDIALSOCKET_MAXNUM_TCP_CLIENTS
	#define CXA_WOLFSSLDIALSOCKET_MAXNUM_TCP_CLIENTS		1
#endif

#ifndef CXA_WOLFSSLDIALSOCKET_MAXNUM_TCP_SERVERS
	#define CXA_WOLFSSLDIALSOCKET_MAXNUM_TCP_SERVERS	0
#else
	#error "TCP server support not available"
#endif


// ******** local type definitions ********
typedef struct
{
	cxa_wolfSslDialSocket_network_tcpClient_t client;
	bool isReserved;
}tcpClient_entry_t;


// ******** local function prototypes ********
static void cxa_network_factory_init(void);


// ********  local variable declarations *********
static bool isInit = false;
static aq_telitTsvgModem_t* modem = NULL;

#if CXA_WOLFSSLDIALSOCKET_MAXNUM_TCP_CLIENTS > 0
static tcpClient_entry_t tcpClientMap[CXA_WOLFSSLDIALSOCKET_MAXNUM_TCP_CLIENTS];
#endif


// ******** global function implementations ********
void cxa_wolfSslDialSocket_network_factory_setModem(aq_telitTsvgModem_t *const modemIn)
{
    cxa_assert(modemIn);

    modem = modemIn;
}


cxa_network_tcpClient_t* cxa_network_factory_reserveTcpClient(int threadIdIn)
{
	if( !isInit ) cxa_network_factory_init();

    // make sure we have a modem
    cxa_assert(modem);

	cxa_network_tcpClient_t* retVal = NULL;

	for( size_t i = 0; i < (sizeof(tcpClientMap)/sizeof(*tcpClientMap)); i++ )
	{
		if( !tcpClientMap[i].isReserved )
		{
			tcpClientMap[i].isReserved = true;
			cxa_wolfSslDialSocket_network_tcpClient_init(&tcpClientMap[i].client, modem, threadIdIn);
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
#if CXA_WOLFSSLDIALSOCKET_MAXNUM_TCP_CLIENTS > 0
	for( size_t i = 0; i < (sizeof(tcpClientMap)/sizeof(*tcpClientMap)); i++ )
	{
		tcpClientMap[i].isReserved = false;
	}
#endif

	isInit = true;
}
