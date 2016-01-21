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
#ifndef CXA_NETWORK_TCPCLIENT_H_
#define CXA_NETWORK_TCPCLIENT_H_


// ******** includes ********
#include <stdbool.h>
#include <cxa_array.h>
#include <cxa_ioStream.h>
#include <cxa_timeDiff.h>
#include <cxa_logger_header.h>
#include <cxa_config.h>


// ******** global macro definitions ********
#ifndef CXA_NETWORK_TCPCLIENT_MAXNUM_LISTENERS
	#define CXA_NETWORK_TCPCLIENT_MAXNUM_LISTENERS		1
#endif


// ******** global type definitions *********
/**
 * @public
 * Forward declaration of cxa_network_tcpClient_t object
 */
typedef struct cxa_network_tcpClient cxa_network_tcpClient_t;


/**
 * @public
 */
typedef void (*cxa_network_tcpClient_cb_onConnect_t)(cxa_network_tcpClient_t *const clientIn, void* userVarIn);

/**
 * @public
 */
typedef void (*cxa_network_tcpClient_cb_onConnectFail_t)(cxa_network_tcpClient_t *const clientInIn, void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_network_tcpClient_cb_onDisconnect_t)(cxa_network_tcpClient_t *const clientIn, void* userVarIn);


/**
 * @private
 * Used for network client subclasses
 */
typedef bool (*cxa_network_tcpClient_scm_connectToHost_t)(cxa_network_tcpClient_t *const superIn, char *const hostNameIn, uint16_t portNumIn, bool useTlsIn, uint32_t timeout_msIn);


/**
 * @private
 * Used for network client subclasses
 */
typedef void (*cxa_network_tcpClient_scm_disconnectFromHost_t)(cxa_network_tcpClient_t *const superIn);


/**
 * @private
 * Used for network client subclasses
 */
typedef bool (*cxa_network_tcpClient_scm_isConnected_t)(cxa_network_tcpClient_t *const superIn);


typedef struct
{
	cxa_network_tcpClient_cb_onConnect_t cb_onConnect;
	cxa_network_tcpClient_cb_onConnectFail_t cb_onConnectFail;
	cxa_network_tcpClient_cb_onDisconnect_t cb_onDisconnect;

	void* userVar;
}cxa_network_tcpClient_listenerEntry_t;


/**
 * @private
 */
struct cxa_network_tcpClient
{
	cxa_ioStream_t ioStream;

	// subclass methods
	cxa_network_tcpClient_scm_connectToHost_t scm_connToHost;
	cxa_network_tcpClient_scm_disconnectFromHost_t scm_disconnect;
	cxa_network_tcpClient_scm_isConnected_t scm_isConnected;

	cxa_timeDiff_t td_genPurp;

	cxa_array_t listeners;
	cxa_network_tcpClient_listenerEntry_t listeners_raw[CXA_NETWORK_TCPCLIENT_MAXNUM_LISTENERS];

	cxa_logger_t logger;
};


// ******** global function prototypes ********
/**
 * @private
 */
void cxa_network_tcpClient_init(cxa_network_tcpClient_t *const netClientIn,
							 cxa_network_tcpClient_scm_connectToHost_t scm_connToHostIn,
							 cxa_network_tcpClient_scm_disconnectFromHost_t scm_disconnectIn,
							 cxa_network_tcpClient_scm_isConnected_t scm_isConnected);

/**
 * @public
 */
void cxa_network_tcpClient_addListener(cxa_network_tcpClient_t *const netClientIn,
									cxa_network_tcpClient_cb_onConnect_t cb_onConnectIn,
									cxa_network_tcpClient_cb_onConnectFail_t cb_onConnectFailIn,
									cxa_network_tcpClient_cb_onDisconnect_t cb_onDisconnectIn,
									void* userVarIn);


/**
 * @public
 */
bool cxa_network_tcpClient_connectToHost(cxa_network_tcpClient_t *const netClientIn, char *const hostNameIn, uint16_t portNumIn, bool useTlsIn, uint32_t timeout_msIn);


/**
 * @public
 */
void cxa_network_tcpClient_disconnect(cxa_network_tcpClient_t *const netClientIn);


/**
 * @public
 */
bool cxa_network_tcpClient_isConnected(cxa_network_tcpClient_t *const netClientIn);


/**
 * @public
 */
cxa_ioStream_t* cxa_network_tcpClient_getIoStream(cxa_network_tcpClient_t *const netClientIn);


#endif // CXA_NETWORK_TCPCLIENT_H_
