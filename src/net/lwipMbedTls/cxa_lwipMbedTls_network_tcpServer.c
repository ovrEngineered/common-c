/**
 * Copyright 2016 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Christopher Armenio
 */
#include <cxa_lwipMbedTls_network_tcpServer.h>


// ******** includes ********
#include <string.h>

#include <cxa_assert.h>
#include <cxa_numberUtils.h>
#include <cxa_stringUtils.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_INFO
#include <cxa_logger_implementation.h>

#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/netdb.h>
#include <lwip/dns.h>
#include <lwip/api.h>


// ******** local macro definitions ********
#define CONNECTION_BACKLOG			1


// ******** local type definitions ********
typedef enum
{
	STATE_IDLE,
	STATE_LISTENING,
	STATE_LISTENING_FAIL
}state_t;


// ******** local function prototypes ********
static bool hasFreeConnectedClient(cxa_lwipMbedTls_network_tcpServer_t *const netServerIn);
static bool reserveAndSetupFreeConnectedClient(cxa_lwipMbedTls_network_tcpServer_t *const netServerIn,
												int socketIn, struct sockaddr_in * clientAddressIn);
static void closeAllSockets(cxa_lwipMbedTls_network_tcpServer_t *const netServerIn);

static bool scm_listen(cxa_network_tcpServer_t *const superIn, uint16_t portNumIn);
static void scm_stopListening(cxa_network_tcpServer_t *const superIn);

static void stateCb_listen_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_listen_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_listen_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void* userVarIn);
static void stateCb_listenFail_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_lwipMbedTls_network_tcpServer_init(cxa_lwipMbedTls_network_tcpServer_t *const netServerIn, int threadIdIn)
{
	cxa_assert(netServerIn);

	// initialize our client connections
	for( size_t i = 0; i < sizeof(netServerIn->connectedClients)/sizeof(*netServerIn->connectedClients); i++ )
	{
		cxa_lwipMbedTls_network_tcpServer_connectedClient_initUnbound(&netServerIn->connectedClients[i]);
	}

	// setup our state machine
	cxa_stateMachine_init(&netServerIn->stateMachine, "tcpServer", threadIdIn);
	cxa_stateMachine_addState(&netServerIn->stateMachine, STATE_IDLE, "idle", NULL, NULL, NULL, (void*)netServerIn);
	cxa_stateMachine_addState(&netServerIn->stateMachine, STATE_LISTENING, "listening", stateCb_listen_enter, stateCb_listen_state, stateCb_listen_leave, (void*)netServerIn);
	cxa_stateMachine_addState(&netServerIn->stateMachine, STATE_LISTENING_FAIL, "listenFail", stateCb_listenFail_enter, NULL, NULL, (void*)netServerIn);
	cxa_stateMachine_setInitialState(&netServerIn->stateMachine, STATE_IDLE);

	// initialize our super class
	cxa_network_tcpServer_init(&netServerIn->super, scm_listen, scm_stopListening);
}


// ******** local function implementations ********
static bool hasFreeConnectedClient(cxa_lwipMbedTls_network_tcpServer_t *const netServerIn)
{
	cxa_assert(netServerIn);

	for( size_t i = 0; i < sizeof(netServerIn->connectedClients)/sizeof(*netServerIn->connectedClients); i++ )
	{
		if( netServerIn->connectedClients[i].socket == -1 ) return true;
	}

	return false;
}


static bool reserveAndSetupFreeConnectedClient(cxa_lwipMbedTls_network_tcpServer_t *const netServerIn,
												int socketIn, struct sockaddr_in * clientAddressIn)
{
	cxa_assert(netServerIn);
	cxa_assert(clientAddressIn);

	// find a free client
	cxa_lwipMbedTls_network_tcpServer_connectedClient_t* targetClient = NULL;
	for( size_t i = 0; i < sizeof(netServerIn->connectedClients)/sizeof(*netServerIn->connectedClients); i++ )
	{
		if( !cxa_network_tcpServer_connectedClient_isBound(&netServerIn->connectedClients[i].super) )
		{
			targetClient = &netServerIn->connectedClients[i];
			break;
		}
	}
	if( targetClient == NULL ) return false;
	// if we made it here, we have a free client to configure

	cxa_lwipMbedTls_network_tcpServer_connectedClient_bindToSocket(targetClient, socketIn, clientAddressIn);

	// notify our listeners
	cxa_network_tcpServer_notifyConnect(&netServerIn->super, &targetClient->super);

	return true;
}


static void closeAllSockets(cxa_lwipMbedTls_network_tcpServer_t *const netServerIn)
{
	cxa_assert(netServerIn);

	// first our clients
	for( size_t i = 0; i < sizeof(netServerIn->connectedClients)/sizeof(*netServerIn->connectedClients); i++ )
	{
		if( netServerIn->connectedClients[i].socket != -1 )
		{
			cxa_network_tcpServer_connectedClient_unbindAndClose(&netServerIn->connectedClients[i].super);
		}
	}

	// now our server
	close(netServerIn->listenSocket);
}


static bool scm_listen(cxa_network_tcpServer_t *const superIn, uint16_t portNumIn)
{
	cxa_lwipMbedTls_network_tcpServer_t *netServerIn = (cxa_lwipMbedTls_network_tcpServer_t*)superIn;
	cxa_assert(netServerIn);

	// make sure we're able to listen
	if( cxa_stateMachine_getCurrentState(&netServerIn->stateMachine) != STATE_IDLE )
	{
		cxa_logger_warn(&netServerIn->super.logger, "bad state for listening");
		return false;
	}
	// if we made it here, we can listen...

	// save our references
	netServerIn->portNumber = portNumIn;

	// transition
	cxa_stateMachine_transition(&netServerIn->stateMachine, STATE_LISTENING);
	return true;
}


static void scm_stopListening(cxa_network_tcpServer_t *const superIn)
{
	cxa_lwipMbedTls_network_tcpServer_t *netServerIn = (cxa_lwipMbedTls_network_tcpServer_t*)superIn;
	cxa_assert(netServerIn);

	if( cxa_stateMachine_getCurrentState(&netServerIn->stateMachine) == STATE_IDLE ) return;

	cxa_logger_info(&netServerIn->super.logger, "stopping listening");
	cxa_stateMachine_transition(&netServerIn->stateMachine, STATE_IDLE);
}


static void stateCb_listen_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_lwipMbedTls_network_tcpServer_t *netServerIn = (cxa_lwipMbedTls_network_tcpServer_t*)userVarIn;
	cxa_assert(netServerIn);

	struct sockaddr_in serverAddress;

	// create a socket that we will listen upon
	cxa_logger_debug(&netServerIn->super.logger, "creating socket");
	netServerIn->listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if( netServerIn->listenSocket < 0 )
	{
		cxa_logger_error(&netServerIn->super.logger, "error listening socket: %d %s", netServerIn->listenSocket, strerror(errno));
		cxa_stateMachine_transition(&netServerIn->stateMachine, STATE_LISTENING_FAIL);
		return;
	}

	// set non-blocking mode on our socket
	cxa_logger_debug(&netServerIn->super.logger, "setting socket non-blocking");
	int arg_turnOnNonBlock = 1;
    lwip_ioctl(netServerIn->listenSocket, FIONBIO, &arg_turnOnNonBlock);

	// bind our server socket to a port
    cxa_logger_debug(&netServerIn->super.logger, "bind socket to any address");
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(netServerIn->portNumber);
	int rc = bind(netServerIn->listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
	if( rc < 0 )
	{
		cxa_logger_error(&netServerIn->super.logger, "error listening bind: %d %s", rc, strerror(errno));
		cxa_stateMachine_transition(&netServerIn->stateMachine, STATE_LISTENING_FAIL);
		return;
	}

	// flag the socket as listening for new connections
	cxa_logger_debug(&netServerIn->super.logger, "flagging socket as listening");
	rc = listen(netServerIn->listenSocket, CONNECTION_BACKLOG);
	if( rc < 0 )
	{
		cxa_logger_error(&netServerIn->super.logger, "error listening listen: %d %s", rc, strerror(errno));
		cxa_stateMachine_transition(&netServerIn->stateMachine, STATE_LISTENING_FAIL);
		return;
	}

	cxa_logger_info(&netServerIn->super.logger, "socket ready for non-blocking listen");
}


static void stateCb_listen_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_lwipMbedTls_network_tcpServer_t *netServerIn = (cxa_lwipMbedTls_network_tcpServer_t*)userVarIn;
	cxa_assert(netServerIn);

	struct sockaddr_in clientAddress;
	char str[INET_ADDRSTRLEN];

	// before we listen, see if we have a clientConnection for a potential connection
	if( hasFreeConnectedClient(netServerIn) )
	{
		// listen for a new client connection
		socklen_t clientAddressLength = sizeof(clientAddress);
		int clientSock = accept(netServerIn->listenSocket, (struct sockaddr *)&clientAddress, &clientAddressLength);
		if( clientSock >= 0 )
		{
			// got a client connection
			inet_ntop(AF_INET, &clientAddress.sin_addr, str, INET_ADDRSTRLEN);
			cxa_logger_info(&netServerIn->super.logger, "got connection from %s, configuring client", str);

			reserveAndSetupFreeConnectedClient(netServerIn, clientSock, &clientAddress);
		}
		else if( errno == EWOULDBLOCK )
		{
			// no connection, but keep trying (non-blocking)
		}
		else
		{
			// error listening
			cxa_logger_error(&netServerIn->super.logger, "error listening accept: %d %d %s", netServerIn->listenSocket, errno, strerror(errno));
			cxa_stateMachine_transition(&netServerIn->stateMachine, STATE_LISTENING_FAIL);
			return;
		}
	}
}


static void stateCb_listen_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void* userVarIn)
{
	cxa_lwipMbedTls_network_tcpServer_t *netServerIn = (cxa_lwipMbedTls_network_tcpServer_t*)userVarIn;
	cxa_assert(netServerIn);

	closeAllSockets(netServerIn);
}


static void stateCb_listenFail_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_lwipMbedTls_network_tcpServer_t *netServerIn = (cxa_lwipMbedTls_network_tcpServer_t*)userVarIn;
	cxa_assert(netServerIn);
}
