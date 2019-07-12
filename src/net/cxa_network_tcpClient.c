/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include <cxa_assert.h>
#include <cxa_network_tcpClient.h>

#define CXA_LOG_LEVEL CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_network_tcpClient_init(cxa_network_tcpClient_t *const netClientIn,
							 cxa_network_tcpClient_scm_connectToHost_t scm_connToHostIn,
							 cxa_network_tcpClient_scm_connectToHost_clientCert_t scm_connToHost_clientCertIn,
							 cxa_network_tcpClient_scm_disconnectFromHost_t scm_disconnectIn,
							 cxa_network_tcpClient_scm_isConnected_t scm_isConnected)
{
	cxa_assert(netClientIn);
	cxa_assert((scm_connToHostIn != NULL) || (scm_connToHost_clientCertIn != NULL));
	cxa_assert(scm_disconnectIn);
	cxa_assert(scm_isConnected);

	// save our references
	netClientIn->scm_connToHost = scm_connToHostIn;
	netClientIn->scm_connToHost_clientCert = scm_connToHost_clientCertIn;
	netClientIn->scm_disconnect = scm_disconnectIn;
	netClientIn->scm_isConnected = scm_isConnected;

	// setup our logger
	cxa_logger_init(&netClientIn->logger, "tcpClient");

	// setup our timediff for future use
	cxa_timeDiff_init(&netClientIn->td_genPurp);

	// setup our listener array
	cxa_array_initStd(&netClientIn->listeners, netClientIn->listeners_raw);

	// setup our ioStream (but the actual client is responsible for binding it)
	cxa_ioStream_init(&netClientIn->ioStream);
}


void cxa_network_tcpClient_addListener(cxa_network_tcpClient_t *const netClientIn,
									cxa_network_tcpClient_cb_onConnect_t cb_onConnectIn,
									cxa_network_tcpClient_cb_onConnectFail_t cb_onConnectFailIn,
									cxa_network_tcpClient_cb_onDisconnect_t cb_onDisconnectIn,
									void* userVarIn)
{
	cxa_assert(netClientIn);

	cxa_network_tcpClient_listenerEntry_t newEntry = {.cb_onConnect=cb_onConnectIn, .cb_onConnectFail=cb_onConnectFailIn, .cb_onDisconnect=cb_onDisconnectIn, .userVar=userVarIn};
	cxa_assert( cxa_array_append(&netClientIn->listeners, &newEntry) );
}


bool cxa_network_tcpClient_connectToHost(cxa_network_tcpClient_t *const netClientIn, char *const hostNameIn, uint16_t portNumIn, bool useTlsIn,  uint32_t timeout_msIn)
{
	cxa_assert(netClientIn);
	cxa_assert(hostNameIn);
	cxa_assert(netClientIn->scm_connToHost);

	return netClientIn->scm_connToHost(netClientIn, hostNameIn, portNumIn, useTlsIn, timeout_msIn);
}


bool cxa_network_tcpClient_connectToHost_clientCert(cxa_network_tcpClient_t *const netClientIn, char *const hostNameIn, uint16_t portNumIn,
													const char* serverRootCertIn, size_t serverRootCertLen_bytesIn,
													const char* clientCertIn, size_t clientCertLen_bytesIn,
													const char* clientPrivateKeyIn, size_t clientPrivateKeyLen_bytesIn,
													uint32_t timeout_msIn)
{
	cxa_assert(netClientIn);
	cxa_assert(hostNameIn);
	cxa_assert(netClientIn->scm_connToHost_clientCert);

	return netClientIn->scm_connToHost_clientCert(netClientIn, hostNameIn, portNumIn, serverRootCertIn, serverRootCertLen_bytesIn, clientCertIn, clientCertLen_bytesIn, clientPrivateKeyIn, clientPrivateKeyLen_bytesIn, timeout_msIn);
}


void cxa_network_tcpClient_disconnect(cxa_network_tcpClient_t *const netClientIn)
{
	cxa_assert(netClientIn);

	netClientIn->scm_disconnect(netClientIn);
}


bool cxa_network_tcpClient_isConnected(cxa_network_tcpClient_t *const netClientIn)
{
	cxa_assert(netClientIn);

	return netClientIn->scm_isConnected(netClientIn);
}


cxa_ioStream_t* cxa_network_tcpClient_getIoStream(cxa_network_tcpClient_t *const netClientIn)
{
	cxa_assert(netClientIn);

	return &netClientIn->ioStream;
}


void cxa_network_tcpClient_notify_connect(cxa_network_tcpClient_t *const netClientIn)
{
	cxa_assert(netClientIn);

	cxa_array_iterate(&netClientIn->listeners, currListener, cxa_network_tcpClient_listenerEntry_t)
	{
		if( currListener == NULL ) continue;
		if( currListener->cb_onConnect != NULL ) currListener->cb_onConnect(netClientIn, currListener->userVar);
	}
}


void cxa_network_tcpClient_notify_connectFail(cxa_network_tcpClient_t *const netClientIn)
{
	cxa_assert(netClientIn);

	cxa_array_iterate(&netClientIn->listeners, currListener, cxa_network_tcpClient_listenerEntry_t)
	{
		if( currListener == NULL ) continue;
		if( currListener->cb_onConnectFail != NULL ) currListener->cb_onConnectFail(netClientIn, currListener->userVar);
	}
}


void cxa_network_tcpClient_notify_disconnect(cxa_network_tcpClient_t *const netClientIn)
{
	cxa_assert(netClientIn);

	cxa_array_iterate(&netClientIn->listeners, currListener, cxa_network_tcpClient_listenerEntry_t)
	{
		if( currListener == NULL ) continue;
		if( currListener->cb_onDisconnect != NULL ) currListener->cb_onDisconnect(netClientIn, currListener->userVar);
	}
}


// ******** local function implementations ********
