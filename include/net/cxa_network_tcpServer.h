/**
 * Copyright 2015 opencxa.org
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
#ifndef CXA_NETWORK_TCPSERVER_H_
#define CXA_NETWORK_TCPSERVER_H_


// ******** includes ********
#include <stdbool.h>
#include <cxa_array.h>
#include <cxa_ioStream.h>
#include <cxa_timeDiff.h>
#include <cxa_logger_header.h>
#include <cxa_config.h>


// ******** global macro definitions ********
#ifndef CXA_NETWORK_TCPSERVER_MAXNUM_LISTENERS
	#define CXA_NETWORK_TCPSERVER_MAXNUM_LISTENERS		1
#endif


// ******** global type definitions *********
/**
 * @public
 * Forward declaration of cxa_network_tcpServer_t object
 */
typedef struct cxa_network_tcpServer cxa_network_tcpServer_t;


/**
 * @public
 */
typedef void (*cxa_network_tcpServer_cb_onConnect_t)(cxa_network_tcpServer_t *const serverIn, void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_network_tcpServer_cb_onDisconnect_t)(cxa_network_tcpServer_t *const serverIn, void* userVarIn);


/**
 * @private
 * Used for subclasses
 */
typedef bool (*cxa_network_tcpServer_cb_listen_t)(cxa_network_tcpServer_t *const superIn, uint16_t portNumIn);


/**
 * @private
 * Used for subclasses
 */
typedef void (*cxa_network_tcpServer_cb_disconnectFromClient_t)(cxa_network_tcpServer_t *const superIn);


/**
 * @private
 * Used for subclasses
 */
typedef bool (*cxa_network_tcpServer_cb_isConnected_t)(cxa_network_tcpServer_t *const superIn);


typedef struct
{
	cxa_network_tcpServer_cb_onConnect_t cb_onConnect;
	cxa_network_tcpServer_cb_onDisconnect_t cb_onDisconnect;

	void* userVar;
}cxa_network_tcpServer_listenerEntry_t;


/**
 * @private
 */
struct cxa_network_tcpServer
{
	cxa_ioStream_t ioStream;

	// subclass methods
	cxa_network_tcpServer_cb_listen_t cb_listen;
	cxa_network_tcpServer_cb_disconnectFromClient_t cb_disconnectFromClient;
	cxa_network_tcpServer_cb_isConnected_t cb_isConnected;

	cxa_array_t listeners;
	cxa_network_tcpServer_listenerEntry_t listeners_raw[CXA_NETWORK_TCPSERVER_MAXNUM_LISTENERS];

	cxa_logger_t logger;
};


// ******** global function prototypes ********
/**
 * @private
 */
void cxa_network_tcpServer_init(cxa_network_tcpServer_t *const tcpServerIn,
								cxa_network_tcpServer_cb_listen_t cb_listenIn,
								cxa_network_tcpServer_cb_disconnectFromClient_t cb_disconnectFromClientIn,
								cxa_network_tcpServer_cb_isConnected_t cb_isConnected);

/**
 * @public
 */
void cxa_network_tcpServer_addListener(cxa_network_tcpServer_t *const tcpServerIn,
									   cxa_network_tcpServer_cb_onConnect_t cb_onConnectIn,
									   cxa_network_tcpServer_cb_onDisconnect_t cb_onDisconnectIn,
									   void* userVarIn);


/**
 * @public
 */
bool cxa_network_tcpServer_listen(cxa_network_tcpServer_t *const tcpServerIn, uint16_t portNumIn);


/**
 * @public
 */
void cxa_network_tcpServer_disconnect(cxa_network_tcpServer_t *const tcpServerIn);


/**
 * @public
 */
bool cxa_network_tcpServer_isConnected(cxa_network_tcpServer_t *const tcpServerIn);


/**
 * @public
 */
cxa_ioStream_t* cxa_network_tcpServer_getIoStream(cxa_network_tcpServer_t *const tcpServerIn);


#endif // CXA_NETWORK_TCPSERVER_H_
