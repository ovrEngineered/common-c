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
#include <cxa_esp8266_network_tcpClient.h>


// ******** includes ********
#include <string.h>

#include <cxa_assert.h>
#include <cxa_numberUtils.h>
#include <cxa_stringUtils.h>
#include <cxa_uniqueId.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>

#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/netdb.h>
#include <lwip/dns.h>
#include <lwip/api.h>


// ******** local macro definitions ********
#define DEBUG_LEVEL 4


// ******** local type definitions ********
typedef enum
{
	STATE_IDLE,
	STATE_CONNECTING,
	STATE_CONNECTED,
	STATE_CONNECT_FAIL
}state_t;


// ******** local function prototypes ********
static bool scm_connectToHost_clientCert(cxa_network_tcpClient_t *const superIn, char *const hostNameIn, uint16_t portNumIn,
																	 const char* serverRootCertIn, size_t serverRootCertLen_bytesIn,
																	 const char* clientCertIn, size_t clientCertLen_bytesIn,
																	 const char* clientPrivateKeyIn, size_t clientPrivateKeyLen_bytesIn,
																	 uint32_t timeout_msIn);
static void scm_disconnectFromHost(cxa_network_tcpClient_t *const superIn);
static bool scm_isConnected(cxa_network_tcpClient_t *const superIn);

static void stateCb_connecting_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_connected_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void* userVarIn);
static void stateCb_connectFail_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);

static cxa_ioStream_readStatus_t cb_ioStream_readByte(uint8_t *const byteOut, void *const userVarIn);
static bool cb_ioStream_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn);


static void netconn_tls_debug(void *ctx, int level,
							  const char *file, int line,
							  const char *str);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_esp8266_network_tcpClient_init(cxa_esp8266_network_tcpClient_t *const netClientIn)
{
	cxa_assert(netClientIn);

	// set some defaults
	netClientIn->tls.initState.areBasicsInitialized = false;
	netClientIn->tls.initState.crc_clientCert = 0;
	netClientIn->tls.initState.crc_clientPrivateKey = 0;
	netClientIn->tls.initState.crc_serverRootCert = 0;
	netClientIn->targetHostName[0] = 0;
	netClientIn->targetPortNum[0] = 0;
	netClientIn->useClientCert = false;

	cxa_stateMachine_init(&netClientIn->stateMachine, "tcpClient");
	cxa_stateMachine_addState(&netClientIn->stateMachine, STATE_IDLE, "idle", NULL, NULL, NULL, (void*)netClientIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, STATE_CONNECTING, "connecting", NULL, stateCb_connecting_state, NULL, (void*)netClientIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, STATE_CONNECTED, "connected", stateCb_connected_enter, NULL, stateCb_connected_leave, (void*)netClientIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, STATE_CONNECT_FAIL, "connFail", stateCb_connectFail_enter, NULL, NULL, (void*)netClientIn);
	cxa_stateMachine_setInitialState(&netClientIn->stateMachine, STATE_IDLE);

	// initialize our super class
	cxa_network_tcpClient_init(&netClientIn->super, NULL, scm_connectToHost_clientCert, scm_disconnectFromHost, scm_isConnected);
}


void cxa_esp8266_network_tcpClient_update(cxa_esp8266_network_tcpClient_t *const netClientIn)
{
	cxa_assert(netClientIn);

	cxa_stateMachine_update(&netClientIn->stateMachine);
}


// ******** local function implementations ********
static bool scm_connectToHost_clientCert(cxa_network_tcpClient_t *const superIn, char *const hostNameIn, uint16_t portNumIn,
																	 const char* serverRootCertIn, size_t serverRootCertLen_bytesIn,
																	 const char* clientCertIn, size_t clientCertLen_bytesIn,
																	 const char* clientPrivateKeyIn, size_t clientPrivateKeyLen_bytesIn,
																	 uint32_t timeout_msIn)
{
	cxa_assert(hostNameIn);
	cxa_assert(serverRootCertIn);
	cxa_assert(clientCertIn);
	cxa_assert(clientPrivateKeyIn);

	cxa_esp8266_network_tcpClient_t* netClientIn = (cxa_esp8266_network_tcpClient_t*)superIn;
	cxa_assert(netClientIn);

	// make sure we are currently idle
	if( cxa_stateMachine_getCurrentState(&netClientIn->stateMachine) != STATE_IDLE ) return false;

	int tmpRet;
	uint16_t tmpCrc;

	// initialize our basics if needed
	if( !netClientIn->tls.initState.areBasicsInitialized )
	{
		cxa_logger_trace(&netClientIn->super.logger, "initializing TLS basics");

		// basic initialization
		mbedtls_ssl_init(&netClientIn->tls.sslContext);
		mbedtls_x509_crt_init(&netClientIn->tls.cert_server);
		mbedtls_x509_crt_init(&netClientIn->tls.cert_client);
		mbedtls_pk_init(&netClientIn->tls.client_key_private);
		mbedtls_ctr_drbg_init(&netClientIn->tls.ctr_drbg);
		mbedtls_ssl_config_init(&netClientIn->tls.conf);
		mbedtls_entropy_init(&netClientIn->tls.entropy);


		char* personalizationString = cxa_uniqueId_getHexString();
		tmpRet = mbedtls_ctr_drbg_seed(&netClientIn->tls.ctr_drbg, mbedtls_entropy_func, &netClientIn->tls.entropy,
										   (const unsigned char *)personalizationString, strlen(personalizationString));
		if( tmpRet != 0 )
		{
			cxa_logger_warn(&netClientIn->super.logger, "failed to seed tls drbg: %d", tmpRet);
			return false;
		}
		netClientIn->tls.initState.areBasicsInitialized = true;
	}

	// server CA certificate
	if( (tmpCrc = cxa_numberUtils_crc16_oneShot((void*)serverRootCertIn, serverRootCertLen_bytesIn)) != netClientIn->tls.initState.crc_serverRootCert )
	{
		cxa_logger_trace(&netClientIn->super.logger, "parsing server CA cert");
	    tmpRet = mbedtls_x509_crt_parse(&netClientIn->tls.cert_server, (const unsigned char*)serverRootCertIn, serverRootCertLen_bytesIn);
	    if( tmpRet < 0 )
	    {
	        cxa_logger_warn(&netClientIn->super.logger, "failed to parse server CA cert: %d", tmpRet);
	        return false;
	    }
	    netClientIn->tls.initState.crc_serverRootCert = tmpCrc;
	}

	// client certificate
	if( (tmpCrc = cxa_numberUtils_crc16_oneShot((void*)clientCertIn, clientCertLen_bytesIn)) != netClientIn->tls.initState.crc_clientCert )
	{
		cxa_logger_trace(&netClientIn->super.logger, "parsing client certificate");
		tmpRet = mbedtls_x509_crt_parse(&netClientIn->tls.cert_client, (const unsigned char*)clientCertIn, clientCertLen_bytesIn);
		if( tmpRet < 0)
		{
			cxa_logger_warn(&netClientIn->super.logger, "failed to parse client cert: %d", tmpRet);
			return false;
		}
		netClientIn->tls.initState.crc_clientCert = tmpCrc;
	}

	// client privatekey
	if( (tmpCrc = cxa_numberUtils_crc16_oneShot((void*)clientPrivateKeyIn, clientPrivateKeyLen_bytesIn)) != netClientIn->tls.initState.crc_clientPrivateKey )
	{
		cxa_logger_trace(&netClientIn->super.logger, "parsing client private key");
		tmpRet = mbedtls_pk_parse_key(&netClientIn->tls.client_key_private, (const unsigned char*)clientPrivateKeyIn, clientPrivateKeyLen_bytesIn, NULL, 0);
		if( tmpRet != 0 )
		{
			cxa_logger_warn(&netClientIn->super.logger, "failed to parse client private key: %d", tmpRet);
			return false;
		}
		netClientIn->tls.initState.crc_clientPrivateKey = tmpCrc;
	}

	// server hostname
	if( !cxa_stringUtils_equals_ignoreCase(hostNameIn, netClientIn->targetHostName) )
	{
		cxa_logger_trace(&netClientIn->super.logger, "setting tls hostname");
		tmpRet = mbedtls_ssl_set_hostname(&netClientIn->tls.sslContext, hostNameIn);
	    if( tmpRet < 0 )
	    {
	    	cxa_logger_warn(&netClientIn->super.logger, "failed to set tls hostname: %d", tmpRet);
			return false;
	    }
	    strlcpy(netClientIn->targetHostName, hostNameIn, sizeof(netClientIn->targetHostName));
	}

	// SSL configuration
	cxa_logger_trace(&netClientIn->super.logger, "configuring tls context");
	tmpRet = mbedtls_ssl_config_defaults(&netClientIn->tls.conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
	if( tmpRet < 0 )
	{
		cxa_logger_warn(&netClientIn->super.logger, "tls configuration failed: %d", tmpRet);
		return false;
	}
	cxa_logger_trace(&netClientIn->super.logger, "1");
    mbedtls_ssl_conf_authmode(&netClientIn->tls.conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
	mbedtls_ssl_conf_ca_chain(&netClientIn->tls.conf, &netClientIn->tls.cert_server, NULL);
	mbedtls_ssl_conf_rng(&netClientIn->tls.conf, mbedtls_ctr_drbg_random, &netClientIn->tls.ctr_drbg);
	mbedtls_ssl_conf_own_cert(&netClientIn->tls.conf, &netClientIn->tls.cert_client, &netClientIn->tls.client_key_private);
	cxa_logger_trace(&netClientIn->super.logger, "2");

#ifdef MBEDTLS_DEBUG_C
	cxa_logger_trace(&netClientIn->super.logger, "3");
    mbedtls_debug_set_threshold(DEBUG_LEVEL);
    mbedtls_ssl_conf_dbg(&netClientIn->tls.conf, netconn_tls_debug, (void*)netClientIn);
#endif

    cxa_logger_trace(&netClientIn->super.logger, "4");
	tmpRet = mbedtls_ssl_setup(&netClientIn->tls.sslContext, &netClientIn->tls.conf);
	if( tmpRet < 0 )
	{
		cxa_logger_warn(&netClientIn->super.logger, "tls context configuration failed: %d", tmpRet);
		return false;
	}

	cxa_logger_trace(&netClientIn->super.logger, "5");

	// make sure we record other useful information
	netClientIn->useClientCert = true;
	snprintf(netClientIn->targetPortNum, sizeof(netClientIn->targetPortNum), "%d", portNumIn);
	netClientIn->targetPortNum[sizeof(netClientIn->targetPortNum)-1] = 0;

	// start the connection
	cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_CONNECTING);

	return true;
}


static void scm_disconnectFromHost(cxa_network_tcpClient_t *const superIn)
{
}


static bool scm_isConnected(cxa_network_tcpClient_t *const superIn)
{
	cxa_esp8266_network_tcpClient_t* netClientIn = (cxa_esp8266_network_tcpClient_t*)superIn;
	cxa_assert(netClientIn);

	return (cxa_stateMachine_getCurrentState(&netClientIn->stateMachine) == STATE_CONNECTED);
}


static void stateCb_connecting_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_esp8266_network_tcpClient_t* netClientIn = (cxa_esp8266_network_tcpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	if( netClientIn->useClientCert )
	{
		int tmpRet;
		mbedtls_net_init(&netClientIn->tls.server_fd);

		cxa_logger_trace(&netClientIn->super.logger, "connecting to '%s:%d", netClientIn->targetHostName, netClientIn->targetPortNum);
		tmpRet = mbedtls_net_connect(&netClientIn->tls.server_fd, netClientIn->targetHostName, netClientIn->targetPortNum, MBEDTLS_NET_PROTO_TCP);
		if( tmpRet < 0 )
		{
			cxa_logger_warn(&netClientIn->super.logger, "connect failed: %d", tmpRet);
			cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_CONNECT_FAIL);
			return;
		}

		// set the byte IO callbacks for our context/descriptor
		mbedtls_ssl_set_bio(&netClientIn->tls.sslContext, &netClientIn->tls.server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

		cxa_logger_trace(&netClientIn->super.logger, "performing TLS handshake");
		while( (tmpRet = mbedtls_ssl_handshake(&netClientIn->tls.sslContext)) != 0 )
		{
			if( (tmpRet != MBEDTLS_ERR_SSL_WANT_READ) && (tmpRet != MBEDTLS_ERR_SSL_WANT_WRITE) )
			{
				cxa_logger_warn(&netClientIn->super.logger, "TLS handshake failed: %d", tmpRet);
				cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_CONNECT_FAIL);
				return;
			}
		}

		cxa_logger_trace(&netClientIn->super.logger, "verifying server certificate");
		if( (tmpRet = mbedtls_ssl_get_verify_result(&netClientIn->tls.sslContext)) != 0)
		{
			// for debugging
			//char vrfy_buf[512];
			//mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", tmpRet);
			//printf("%s\n", vrfy_buf);

			cxa_logger_warn(&netClientIn->super.logger, "server certificate verification failed: %d", tmpRet);
			cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_CONNECT_FAIL);
			return;
		}
	}
	else cxa_assert(false);

	cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_CONNECTED);
}


static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_esp8266_network_tcpClient_t* netClientIn = (cxa_esp8266_network_tcpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	// bind our ioStream
	cxa_ioStream_bind(&netClientIn->super.ioStream, cb_ioStream_readByte, cb_ioStream_writeBytes, (void*)netClientIn);

	// notify our listeners
	cxa_array_iterate(&netClientIn->super.listeners, currListener, cxa_network_tcpClient_listenerEntry_t)
	{
		if( currListener == NULL ) continue;
		if( currListener->cb_onConnect != NULL ) currListener->cb_onConnect(&netClientIn->super, currListener->userVar);
	}
}


static void stateCb_connected_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void* userVarIn)
{
	cxa_esp8266_network_tcpClient_t* netClientIn = (cxa_esp8266_network_tcpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	// reset our ssl context
	mbedtls_ssl_session_reset(&netClientIn->tls.sslContext);

	// notify our listeners
	cxa_array_iterate(&netClientIn->super.listeners, currListener, cxa_network_tcpClient_listenerEntry_t)
	{
		if( currListener == NULL ) continue;
		if( currListener->cb_onDisconnect != NULL ) currListener->cb_onDisconnect(&netClientIn->super, currListener->userVar);
	}
}


static void stateCb_connectFail_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_esp8266_network_tcpClient_t* netClientIn = (cxa_esp8266_network_tcpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	// notify our listeners
	cxa_array_iterate(&netClientIn->super.listeners, currListener, cxa_network_tcpClient_listenerEntry_t)
	{
		if( currListener == NULL ) continue;
		if( currListener->cb_onConnectFail != NULL ) currListener->cb_onConnectFail(&netClientIn->super, currListener->userVar);
	}

	cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_IDLE);
}


static cxa_ioStream_readStatus_t cb_ioStream_readByte(uint8_t *const byteOut, void *const userVarIn)
{
	cxa_esp8266_network_tcpClient_t* netClientIn = (cxa_esp8266_network_tcpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	int tmpRet = mbedtls_ssl_read(&netClientIn->tls.sslContext, byteOut, 1);
	if( tmpRet < 0 )
	{
		cxa_logger_warn(&netClientIn->super.logger, "error during read: %d", tmpRet);
		cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_IDLE);
		return CXA_IOSTREAM_READSTAT_ERROR;
	}

	return (tmpRet > 0) ? CXA_IOSTREAM_READSTAT_GOTDATA : CXA_IOSTREAM_READSTAT_NODATA;
}


static bool cb_ioStream_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn)
{
	cxa_esp8266_network_tcpClient_t* netClientIn = (cxa_esp8266_network_tcpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	// handle a zero-size buffer appropriately
	if( bufferSize_bytesIn != 0 ) { cxa_assert(buffIn); }
	else { return true; }

	// make sure we are connected
	if( !cxa_network_tcpClient_isConnected(&netClientIn->super) ) return false;

	int tmpRet;
	unsigned char* buf = buffIn;
	do
	{
		tmpRet = mbedtls_ssl_write(&netClientIn->tls.sslContext, (const unsigned char*)buf, bufferSize_bytesIn);
		if( tmpRet > 0 )
		{
			buf += tmpRet;
			bufferSize_bytesIn -= tmpRet;
		}
		else if( (tmpRet != MBEDTLS_ERR_SSL_WANT_WRITE) && (tmpRet != MBEDTLS_ERR_SSL_WANT_READ) )
		{
			cxa_logger_warn(&netClientIn->super.logger, "error during write: %d", tmpRet);
			cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_IDLE);
			return false;
		}
	} while( bufferSize_bytesIn > 0 );

	return true;
}


static void netconn_tls_debug(void *ctx, int level,
							  const char *file, int line,
							  const char *str)
{
	((void) level);

	cxa_esp8266_network_tcpClient_t* netClientIn = (cxa_esp8266_network_tcpClient_t*)ctx;
	cxa_assert(netClientIn);

    /* Shorten 'file' from the whole file path to just the filename

       This is a bit wasteful because the macros are compiled in with
       the full _FILE_ path in each case, so the firmware is bloated out
       by a few kb. But there's not a lot we can do about it...
    */
    char *file_sep = rindex(file, '/');
    if(file_sep) file = file_sep+1;

    cxa_logger_trace(&netClientIn->super.logger, "%s:%04d: %s", file, line, str);
}
