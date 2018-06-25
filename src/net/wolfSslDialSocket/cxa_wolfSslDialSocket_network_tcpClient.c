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
#include <cxa_wolfSslDialSocket_network_tcpClient.h>


// ******** includes ********
#include <string.h>

#include <cxa_assert.h>
#include <cxa_numberUtils.h>
#include <cxa_stringUtils.h>
#include <cxa_uniqueId.h>

#include <wolfssl/wolfcrypt/logging.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define WRITE_TIMEOUT_MS					20000


// ******** local type definitions ********
typedef enum
{
	STATE_IDLE,
	STATE_CONNECTING,
	STATE_CONNECTED,
	STATE_CONNECT_FAIL
}state_t;


// ******** local function prototypes ********
static void cleanupConnectionResources(cxa_wolfSslDialSocket_network_tcpClient_t *const netClientIn);

static bool scm_connectToHost_clientCert(cxa_network_tcpClient_t *const superIn, char *const hostNameIn, uint16_t portNumIn,
																	 const char* serverRootCertIn, size_t serverRootCertLen_bytesIn,
																	 const char* clientCertIn, size_t clientCertLen_bytesIn,
																	 const char* clientPrivateKeyIn, size_t clientPrivateKeyLen_bytesIn,
																	 uint32_t timeout_msIn);
static void scm_disconnectFromHost(cxa_network_tcpClient_t *const superIn);
static bool scm_isConnected(cxa_network_tcpClient_t *const superIn);

static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_connected_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void* userVarIn);
static void stateCb_connectFail_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);

static cxa_ioStream_readStatus_t cb_ioStream_readByte(uint8_t *const byteOut, void *const userVarIn);
static bool cb_ioStream_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn);

static int wolfSsl_ioRx(WOLFSSL *ssl, char *buf, int sz, void *ctx);
static int wolfSsl_ioTx(WOLFSSL *ssl, char *buf, int sz, void *ctx);

static void wolfSsl_cb_logging(const int logLevel, const char *const logMessage);
static int wolfSsl_cb_verifyCert(int preverify, WOLFSSL_X509_STORE_CTX* store);

static void cb_modem_onSocketConnected(cxa_ioStream_t *const socketIoStreamIn, void *userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_wolfSslDialSocket_network_tcpClient_init(cxa_wolfSslDialSocket_network_tcpClient_t *const netClientIn,
                                                    aq_telitTsvgModem_t *const modemIn,
                                                    int threadIdIn)
{
	cxa_assert(netClientIn);
    cxa_assert(modemIn);

	// set some defaults
    netClientIn->modem = modemIn;
    netClientIn->modemIoStream = NULL;
	netClientIn->tls.initState.areBasicsInitialized = false;
	netClientIn->tls.initState.crc_clientCert = 0;
	netClientIn->tls.initState.crc_clientPrivateKey = 0;
	netClientIn->tls.initState.crc_serverRootCert = 0;
	netClientIn->targetHostName[0] = 0;
	netClientIn->targetPortNum = 0;
	netClientIn->useClientCert = false;

	cxa_stateMachine_init(&netClientIn->stateMachine, "tcpClient", threadIdIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, STATE_IDLE, "idle", NULL, NULL, NULL, (void*)netClientIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, STATE_CONNECTING, "connecting", stateCb_connecting_enter, NULL, NULL, (void*)netClientIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, STATE_CONNECTED, "connected", stateCb_connected_enter, NULL, stateCb_connected_leave, (void*)netClientIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, STATE_CONNECT_FAIL, "connFail", stateCb_connectFail_enter, NULL, NULL, (void*)netClientIn);
	cxa_stateMachine_setInitialState(&netClientIn->stateMachine, STATE_IDLE);

	// initialize our super class
	cxa_network_tcpClient_init(&netClientIn->super, NULL, scm_connectToHost_clientCert, scm_disconnectFromHost, scm_isConnected);
}


// ******** local function implementations ********
static void cleanupConnectionResources(cxa_wolfSslDialSocket_network_tcpClient_t *const netClientIn)
{
	cxa_assert(netClientIn);

	cxa_logger_debug(&netClientIn->super.logger, "cleanupConnectionResources");

	// "Gracefully shutdown the connection and free associated data"
}


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

	cxa_wolfSslDialSocket_network_tcpClient_t* netClientIn = (cxa_wolfSslDialSocket_network_tcpClient_t*)superIn;
	cxa_assert(netClientIn);

	// make sure we are currently idle
	if( cxa_stateMachine_getCurrentState(&netClientIn->stateMachine) != STATE_IDLE )
	{
		cxa_logger_trace(&netClientIn->super.logger, "not idle, cannot connect");
		return false;
	}

	int tmpRet;
	uint16_t tmpCrc;

	// initialize our basics if needed
	if( !netClientIn->tls.initState.areBasicsInitialized )
	{
		cxa_logger_trace(&netClientIn->super.logger, "initializing TLS basics");

		// basic initialization
        wolfSSL_Init();
    
        #if CXA_LOG_LEVEL == CXA_LOG_LEVEL_TRACE
        wolfSSL_Debugging_ON();
        wolfSSL_SetLoggingCb(wolfSsl_cb_logging);
        #endif
    
        // Create and initialize WOLFSSL_CTX
        if( (netClientIn->tls.ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL)
        {
            cxa_logger_warn(&netClientIn->super.logger, "ERROR: failed to create WOLFSSL_CTX");
            return false;
        }
    
        // set io functions
        wolfSSL_SetIORecv(netClientIn->tls.ctx, wolfSsl_ioRx);
        wolfSSL_SetIOSend(netClientIn->tls.ctx, wolfSsl_ioTx);
    
        // put ourselves in the verification flow
        wolfSSL_CTX_set_verify(netClientIn->tls.ctx, SSL_VERIFY_NONE, wolfSsl_cb_verifyCert);

		netClientIn->tls.initState.areBasicsInitialized = true;
	}

	// server CA certificate
	if( (tmpCrc = cxa_numberUtils_crc16_oneShot((void*)serverRootCertIn, serverRootCertLen_bytesIn)) != netClientIn->tls.initState.crc_serverRootCert )
	{
		cxa_logger_trace(&netClientIn->super.logger, "parsing server CA cert");

        if( (tmpRet = wolfSSL_CTX_load_verify_buffer(netClientIn->tls.ctx, serverRootCertIn, serverRootCertLen_bytesIn, SSL_FILETYPE_PEM)) != SSL_SUCCESS )
        {
            cxa_logger_warn(&netClientIn->super.logger, "failed to parse server CA cert: %s0x%x", tmpRet<0?"-":"", tmpRet<0?-(unsigned)tmpRet:tmpRet);
            return false;
        }
        
	    netClientIn->tls.initState.crc_serverRootCert = tmpCrc;
	}

	// client certificate
	if( (tmpCrc = cxa_numberUtils_crc16_oneShot((void*)clientCertIn, clientCertLen_bytesIn)) != netClientIn->tls.initState.crc_clientCert )
	{
		cxa_logger_trace(&netClientIn->super.logger, "parsing client certificate");

        if( (tmpRet = wolfSSL_CTX_use_certificate_buffer(netClientIn->tls.ctx, clientCertIn, clientCertLen_bytesIn, SSL_FILETYPE_PEM)) != SSL_SUCCESS )
        {
            cxa_logger_warn(&netClientIn->super.logger, "failed to parse client cert: %s0x%x", tmpRet<0?"-":"", tmpRet<0?-(unsigned)tmpRet:tmpRet);
			return false;
        }
        
		netClientIn->tls.initState.crc_clientCert = tmpCrc;
	}

	// client privatekey
	if( (tmpCrc = cxa_numberUtils_crc16_oneShot((void*)clientPrivateKeyIn, clientPrivateKeyLen_bytesIn)) != netClientIn->tls.initState.crc_clientPrivateKey )
	{
		cxa_logger_trace(&netClientIn->super.logger, "parsing client private key");

        if( (tmpRet = wolfSSL_CTX_use_PrivateKey_buffer(netClientIn->tls.ctx, clientPrivateKeyIn, clientPrivateKeyLen_bytesIn, SSL_FILETYPE_PEM)) != SSL_SUCCESS )
        {
            cxa_logger_warn(&netClientIn->super.logger, "failed to parse client private key: %s0x%x", tmpRet<0?"-":"", tmpRet<0?-(unsigned)tmpRet:tmpRet);
			return false;
        }
        
		netClientIn->tls.initState.crc_clientPrivateKey = tmpCrc;
	}

	// server hostname and port
	if( !cxa_stringUtils_equals_ignoreCase(hostNameIn, netClientIn->targetHostName) )
	{
		cxa_logger_trace(&netClientIn->super.logger, "setting tls hostname");
	    if( !cxa_stringUtils_copy(netClientIn->targetHostName, hostNameIn, sizeof(netClientIn->targetHostName)) )
        {
            cxa_logger_warn(&netClientIn->super.logger, "hostname too long, increase 'CXA_WOLFSSLDIALSOCKET_NETWORK_TCPCLIENT_MAXHOSTNAMELEN_BYTES'");
            return false;    
        }
	}
    netClientIn->targetPortNum = portNumIn;

	// SSL configuration
	cxa_logger_trace(&netClientIn->super.logger, "configuring ssl object");
    if( (netClientIn->tls.ssl = wolfSSL_new(netClientIn->tls.ctx)) == NULL )
    {
        cxa_logger_warn(&netClientIn->super.logger, "failed to configure ssl object");
        return false;
    }
    wolfSSL_SetIOReadCtx(netClientIn->tls.ssl, (void*)netClientIn);
    wolfSSL_SetIOWriteCtx(netClientIn->tls.ssl, (void*)netClientIn);
    
	// make sure we record other useful information
	netClientIn->useClientCert = true;

	// start the connection
	cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_CONNECTING);
    
	return true;
}


static void scm_disconnectFromHost(cxa_network_tcpClient_t *const superIn)
{
	cxa_wolfSslDialSocket_network_tcpClient_t* netClientIn = (cxa_wolfSslDialSocket_network_tcpClient_t*)superIn;
	cxa_assert(netClientIn);

	cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_IDLE);
}


static bool scm_isConnected(cxa_network_tcpClient_t *const superIn)
{
	cxa_wolfSslDialSocket_network_tcpClient_t* netClientIn = (cxa_wolfSslDialSocket_network_tcpClient_t*)superIn;
	cxa_assert(netClientIn);

	return (cxa_stateMachine_getCurrentState(&netClientIn->stateMachine) == STATE_CONNECTED);
}


static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_wolfSslDialSocket_network_tcpClient_t* netClientIn = (cxa_wolfSslDialSocket_network_tcpClient_t*)userVarIn;
	cxa_assert(netClientIn);

    cxa_logger_info(&netClientIn->super.logger, "connecting to '%s:%d", netClientIn->targetHostName, netClientIn->targetPortNum);
        
    // start our modem connecting
    if( !aq_telitTsvgModem_openSocket(netClientIn->modem, netClientIn->targetHostName, netClientIn->targetPortNum, cb_modem_onSocketConnected, (void*)netClientIn) )
    {
        cxa_logger_warn(&netClientIn->super.logger, "failed to initialize dialSocket");
        cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_CONNECT_FAIL);
        return;
    }
}


static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_wolfSslDialSocket_network_tcpClient_t* netClientIn = (cxa_wolfSslDialSocket_network_tcpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	// make sure our socket is non-blocking now

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
	cxa_wolfSslDialSocket_network_tcpClient_t* netClientIn = (cxa_wolfSslDialSocket_network_tcpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	cleanupConnectionResources(netClientIn);

	// notify our listeners
	cxa_array_iterate(&netClientIn->super.listeners, currListener, cxa_network_tcpClient_listenerEntry_t)
	{
		if( currListener == NULL ) continue;
		if( currListener->cb_onDisconnect != NULL ) currListener->cb_onDisconnect(&netClientIn->super, currListener->userVar);
	}
}


static void stateCb_connectFail_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_wolfSslDialSocket_network_tcpClient_t* netClientIn = (cxa_wolfSslDialSocket_network_tcpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	cleanupConnectionResources(netClientIn);

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
	cxa_wolfSslDialSocket_network_tcpClient_t* netClientIn = (cxa_wolfSslDialSocket_network_tcpClient_t*)userVarIn;
	cxa_assert(netClientIn);

    int tmpRet = wolfSSL_read(netClientIn->tls.ssl, byteOut, 1);
    return  (tmpRet == SSL_ERROR_WANT_READ) ? 
            CXA_IOSTREAM_READSTAT_NODATA :
            ((tmpRet == 1) ? CXA_IOSTREAM_READSTAT_GOTDATA : CXA_IOSTREAM_READSTAT_ERROR);
}


static bool cb_ioStream_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn)
{
	cxa_wolfSslDialSocket_network_tcpClient_t* netClientIn = (cxa_wolfSslDialSocket_network_tcpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	// handle a zero-size buffer appropriately
	if( bufferSize_bytesIn != 0 ) { cxa_assert(buffIn); }
	else { return true; }

	// make sure we are connected
	if( !cxa_network_tcpClient_isConnected(&netClientIn->super) ) return false;

    return (wolfSSL_write(netClientIn->tls.ssl, buffIn, bufferSize_bytesIn) == bufferSize_bytesIn);
}


static int wolfSsl_ioRx(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
    cxa_wolfSslDialSocket_network_tcpClient_t* netClientIn = (cxa_wolfSslDialSocket_network_tcpClient_t*)ctx;
	cxa_assert(netClientIn);
    
    uint8_t rxByte;
    int numRxBytes = 0;
    
    // only try for a short time before yielding to others
    cxa_timeDiff_t td_timeout;
    cxa_timeDiff_init(&td_timeout);
    
    while( numRxBytes < sz )
    {
        if( cxa_ioStream_readByte(netClientIn->modemIoStream, &rxByte) == CXA_IOSTREAM_READSTAT_GOTDATA )
        {
            buf[numRxBytes] = rxByte;
            numRxBytes++;
        }
        else
        {
            if( cxa_timeDiff_isElapsed_ms(&td_timeout, 1000) ) break;
        }
    }
    
    cxa_logger_trace(&netClientIn->super.logger, "read %d / %d bytes", numRxBytes, sz);

    return numRxBytes;
}


static int wolfSsl_ioTx(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
    cxa_wolfSslDialSocket_network_tcpClient_t* netClientIn = (cxa_wolfSslDialSocket_network_tcpClient_t*)ctx;
	cxa_assert(netClientIn);
    
    cxa_logger_trace(&netClientIn->super.logger, "wants to write %d bytes", sz);
    
    cxa_ioStream_writeBytes(netClientIn->modemIoStream, buf, sz);
    
    return sz;
}


static void wolfSsl_cb_logging(const int logLevel, const char *const logMessage)
{
    static cxa_logger_t logger;
    static bool isLoggerInit = false;
    if( !isLoggerInit ) { cxa_logger_init(&logger, "wolfSSL"); isLoggerInit = true; }
    
    cxa_logger_debug(&logger, "lvl:%d   msg:%s", logLevel, logMessage);
}


static int wolfSsl_cb_verifyCert(int preverify, WOLFSSL_X509_STORE_CTX* store)
{
    return 1;
}


static void cb_modem_onSocketConnected(cxa_ioStream_t *const socketIoStreamIn, void *userVarIn)
{
    cxa_wolfSslDialSocket_network_tcpClient_t* netClientIn = (cxa_wolfSslDialSocket_network_tcpClient_t*)userVarIn;
	cxa_assert(netClientIn);
    
    // save our ioStream
    netClientIn->modemIoStream = socketIoStreamIn;
    
    if( netClientIn->useClientCert )
	{
        int tmpRet;
        
        cxa_logger_trace(&netClientIn->super.logger, "performing tls handshake...");
        if( (tmpRet = wolfSSL_connect(netClientIn->tls.ssl)) != SSL_SUCCESS )
        {
            cxa_logger_warn(&netClientIn->super.logger, "failed tls handshake: %s0x%x", tmpRet<0?"-":"", tmpRet<0?-(unsigned)tmpRet:tmpRet);
            cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_CONNECT_FAIL);
            return;
        }
	}
	else cxa_assert(false);

	cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_CONNECTED);
}