/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_LWIPMBEDTLS_NETWORK_TCPSERVER_H_
#define CXA_LWIPMBEDTLS_NETWORK_TCPSERVER_H_


// ******** includes ********
#include <cxa_network_tcpServer.h>

#include <cxa_ioStream.h>
#include <cxa_lwipMbedTls_network_tcpServer_connectedClient.h>
#include <cxa_stateMachine.h>
#include <cxa_timeDiff.h>

#include <lwip/sockets.h>


// ******** global macro definitions ********
#ifndef CXA_LWIPMBEDTLS_NETWORK_TCPSERVER_MAXCONNECTEDCLIENTS
	#define CXA_LWIPMBEDTLS_NETWORK_TCPSERVER_MAXCONNECTEDCLIENTS			2
#endif


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_lwipMbedTls_network_tcpServer_t object
 */
typedef struct cxa_lwipMbedTls_network_tcpServer cxa_lwipMbedTls_network_tcpServer_t;


/**
 * @private
 */
struct cxa_lwipMbedTls_network_tcpServer
{
	cxa_network_tcpServer_t super;

	uint16_t portNumber;
	int listenSocket;

	cxa_lwipMbedTls_network_tcpServer_connectedClient_t connectedClients[CXA_LWIPMBEDTLS_NETWORK_TCPSERVER_MAXCONNECTEDCLIENTS];

	cxa_stateMachine_t stateMachine;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_lwipMbedTls_network_tcpServer_init(cxa_lwipMbedTls_network_tcpServer_t *const netServerIn, int threadIdIn);


#endif // CXA_LWIPMBEDTLS_NETWORK_TCPSERVER_H_
