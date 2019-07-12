/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include <cxa_network_httpClient.h>


// ******** includes ********
#include <string.h>

#include <cxa_assert.h>
#include <cxa_network_factory.h>
#include <cxa_stringUtils.h>

#define CXA_LOG_LEVEL CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define MAXNUM_RX_BYTES_PER_ITERATION			8


// ******** local type definitions ********
typedef enum
{
	STATE_IDLE_DISCONNECTED,
	STATE_IDLE_CONNECTED,
	STATE_CONNECTING,
	STATE_CONNECTED_SEND_DEFAULT_HEADERS,
	STATE_CONNECTED_CALC_USER_BODY,
	STATE_CONNECTED_GEN_USER_HEADERS,
	STATE_CONNECTED_GEN_USER_BODY,
	STATE_CONNECTED_PARSE_STATUS_CODE,
	STATE_CONNECTED_PARSE_CONTENT_LENGTH,
	STATE_CONNECTED_FORWARD_TO_BODY,
	STATE_CONNECTED_READ_BODY,
	STATE_WAIT_DISCONNECT,
	STATE_TRANSACTION_ERROR,
}state_t;


// ******** local function prototypes ********
static bool parseStatusCodeFromString(char *const lineIn, uint16_t* statusCodeOut);
static bool parseContentLengthFromString(char *const lineIn, size_t *const contentLength_bytesOut);

static void stateCb_idle_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_sendDefaultHeaders_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_genUserHeaders_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_calcUserBody_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_genUserBody_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_xxxUserBody_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_parseStatusCode_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_xxxCheckTimeout_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_parseContentLength_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_forwardToBody_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_readBody_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_readBody_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_waitDisconnect_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_waitDisconnect_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_transactionError_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);

static void cb_tcpClient_onConnect(cxa_network_tcpClient_t *const superIn, void* userVarIn);
static void cb_tcpClient_onConnectFail(cxa_network_tcpClient_t *const tcpClientIn, void* userVarIn);
static void cb_tcpClient_onDisconnect(cxa_network_tcpClient_t *const superIn, void* userVarIn);

static void cb_headerParser_onIoException(void *const userVarIn);
static void cb_headerParser_onReceptionTimeout(cxa_fixedByteBuffer_t *const incompletePacketIn, void *const userVarIn);
static void cb_headerParser_onPacketReceived(cxa_fixedByteBuffer_t *const packetIn, void *const userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_network_httpClient_init(cxa_network_httpClient_t *const netClientIn, int threadIdIn)
{
	cxa_assert(netClientIn);

	cxa_logger_init(&netClientIn->logger, "httpClient");

	// try to get a client we can use
	netClientIn->tcpClient = cxa_network_factory_reserveTcpClient(threadIdIn);
	cxa_assert(netClientIn->tcpClient);
	cxa_network_tcpClient_addListener(netClientIn->tcpClient, cb_tcpClient_onConnect, cb_tcpClient_onConnectFail, cb_tcpClient_onDisconnect, (void*)netClientIn);

	// setup for body generation
	cxa_ioStream_nullablePassthrough_init(&netClientIn->ios_bodyGeneration);

	// setup for responses
	cxa_fixedByteBuffer_initStd(&netClientIn->headerLineBuffer, netClientIn->headerLineBuffer_raw);
	cxa_protocolParser_crlf_init(&netClientIn->headerLineParser, cxa_network_tcpClient_getIoStream(netClientIn->tcpClient), &netClientIn->headerLineBuffer, threadIdIn);
	cxa_protocolParser_addProtocolListener(&netClientIn->headerLineParser.super, cb_headerParser_onIoException, cb_headerParser_onReceptionTimeout, (void*)netClientIn);
	cxa_protocolParser_addPacketListener(&netClientIn->headerLineParser.super, cb_headerParser_onPacketReceived, (void*)netClientIn);
	cxa_timeDiff_init(&netClientIn->td_receptionTimeout);

	// setup our state machine
	cxa_stateMachine_init(&netClientIn->stateMachine, "httpClient", threadIdIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, STATE_IDLE_DISCONNECTED, "idle (disconn)", stateCb_idle_enter, NULL, NULL, (void*)netClientIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, STATE_IDLE_CONNECTED, "idle (conn)", stateCb_idle_enter, NULL, NULL, (void*)netClientIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, STATE_CONNECTING, "connecting", stateCb_connecting_enter, NULL, NULL, (void*)netClientIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, STATE_CONNECTED_SEND_DEFAULT_HEADERS, "sendDefaultHead", stateCb_sendDefaultHeaders_enter, NULL, NULL, (void*)netClientIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, STATE_CONNECTED_CALC_USER_BODY, "calcBody", stateCb_calcUserBody_enter, stateCb_xxxUserBody_state, NULL, (void*)netClientIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, STATE_CONNECTED_GEN_USER_HEADERS, "genHeaders", NULL, stateCb_genUserHeaders_state, NULL, (void*)netClientIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, STATE_CONNECTED_GEN_USER_BODY, "genBody", stateCb_genUserBody_enter, stateCb_xxxUserBody_state, NULL, (void*)netClientIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, STATE_CONNECTED_PARSE_STATUS_CODE, "parseStatusCode", stateCb_parseStatusCode_enter, stateCb_xxxCheckTimeout_state, NULL, (void*)netClientIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, STATE_CONNECTED_PARSE_CONTENT_LENGTH, "parseContentLength", stateCb_parseContentLength_enter, stateCb_xxxCheckTimeout_state, NULL, (void*)netClientIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, STATE_CONNECTED_FORWARD_TO_BODY, "forwardToBody", stateCb_forwardToBody_enter, stateCb_xxxCheckTimeout_state, NULL, (void*)netClientIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, STATE_CONNECTED_READ_BODY, "readBody", stateCb_readBody_enter, stateCb_readBody_state, NULL, (void*)netClientIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, STATE_WAIT_DISCONNECT, "waitDisconn", stateCb_waitDisconnect_enter, stateCb_waitDisconnect_state, NULL, (void*)netClientIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, STATE_TRANSACTION_ERROR, "transError", stateCb_transactionError_enter, NULL, NULL, (void*)netClientIn);
	cxa_stateMachine_setInitialState(&netClientIn->stateMachine, STATE_IDLE_DISCONNECTED);
}


void cxa_network_httpClient_post_async(cxa_network_httpClient_t *const netClientIn,
								 	   char *const hostNameIn, uint16_t portNumIn, bool useTlsIn,
									   const char *const urlIn, uint32_t timeout_msIn, bool keepOpenIn,
									   cxa_network_httpClient_cb_asyncGenHeaders_t cb_genHeadersIn,
									   cxa_network_httpClient_cb_postAsyncGenBody_t cb_genBodyIn,
									   cxa_network_httpClient_cb_onPostComplete_t cb_postCompleteIn,
									   uint8_t *const responseBodyBufferIn, size_t responseBody_maxSize_bytesIn,
									   void* userVarIn)
{
	cxa_assert(netClientIn);
	cxa_assert(hostNameIn);
	cxa_assert(urlIn);
	if( responseBody_maxSize_bytesIn > 0 ) cxa_assert(responseBodyBufferIn);

	// save our info for later
	netClientIn->cbs.genHeaders = cb_genHeadersIn;
	netClientIn->cbs.genBody = cb_genBodyIn;
	netClientIn->cbs.postComplete = cb_postCompleteIn;
	netClientIn->cbs.userVar = userVarIn;

	cxa_assert(cxa_stringUtils_copy(netClientIn->hostname, hostNameIn, sizeof(netClientIn->hostname)));
	netClientIn->portNum = portNumIn;
	netClientIn->useTls = useTlsIn;
	cxa_assert(cxa_stringUtils_copy(netClientIn->url, urlIn, sizeof(netClientIn->url)));
	netClientIn->timeout_ms = timeout_msIn;
	netClientIn->keepOpen = keepOpenIn;

	netClientIn->responseBodyBuffer = responseBodyBufferIn;
	netClientIn->responseBody_maxSize_bytes = responseBody_maxSize_bytesIn;

	// start our connection process
	cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_CONNECTING);
}


// ******** local function implementations ********
static bool parseStatusCodeFromString(char *const lineIn, uint16_t* statusCodeOut)
{
	char* save_ptr;
	char* currToken = strtok_r(lineIn, " ", &save_ptr);
	if( currToken == NULL ) return false;

	// first token should start with 'HTTP'
	const char httpStr[] = "HTTP";
	if( strncmp(currToken, httpStr, strlen(httpStr)) != 0 ) return false;

	// next token should be status code
	currToken = strtok_r(NULL, " ", &save_ptr);
	if( currToken == NULL ) return false;

	char* endPtr;
	long statusCode = strtol(currToken, &endPtr, 10);
	if( *endPtr != '\0' ) return false;

	if( statusCodeOut != NULL ) *statusCodeOut = (uint16_t)statusCode;
	return true;
}


static bool parseContentLengthFromString(char *const lineIn, size_t *const contentLength_bytesOut)
{
	const char contentLenStr[] = "Content-Length:";

	// if we made it here we got a line...see if it starts with the right string

	if( !cxa_stringUtils_startsWith(lineIn, contentLenStr) ) return false;

	// if we made it here, we got the right line...parse it
	char* save_ptr;
	char* currToken = strtok_r(lineIn, " ", &save_ptr);
	if( currToken == NULL ) return false;

	// one more tokenization should give us the value
	currToken = strtok_r(NULL, " ", &save_ptr);
	if( currToken == NULL ) return false;

	long contentLength = strtol(currToken, NULL, 10);

	if( contentLength_bytesOut != NULL ) *contentLength_bytesOut = (size_t)contentLength;

	return true;
}


static void stateCb_idle_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_network_httpClient_t *const netClientIn = (cxa_network_httpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	state_t currState = cxa_stateMachine_getCurrentState(&netClientIn->stateMachine);

	// see if we need to disconnect
	if( (currState == STATE_IDLE_DISCONNECTED) && cxa_network_tcpClient_isConnected(netClientIn->tcpClient) )
	{
		cxa_network_tcpClient_disconnect(netClientIn->tcpClient);
	}

	cxa_logger_debug(&netClientIn->logger, "idle (%s)",
					(currState == STATE_IDLE_CONNECTED) ?
					"connected" : "disconnected");
}


static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_network_httpClient_t *const netClientIn = (cxa_network_httpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	cxa_logger_info(&netClientIn->logger, "connecting to %s::%d", netClientIn->hostname, netClientIn->portNum);
	if( !cxa_network_tcpClient_connectToHost(netClientIn->tcpClient, netClientIn->hostname, netClientIn->portNum, netClientIn->useTls, netClientIn->timeout_ms) )
	{
		cxa_logger_warn(&netClientIn->logger, "error initiating connection");
		cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_TRANSACTION_ERROR);
		return;
	}

	// wait for tcpClient callbacks
}


static void stateCb_sendDefaultHeaders_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_network_httpClient_t *const netClientIn = (cxa_network_httpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	cxa_logger_debug(&netClientIn->logger, "connected, sending default headers");

	cxa_ioStream_t* ios = cxa_network_tcpClient_getIoStream(netClientIn->tcpClient);

	cxa_ioStream_writeFormattedLine(ios, "POST %s HTTP/1.1", netClientIn->url);
	cxa_ioStream_writeFormattedLine(ios, "Host: %s", netClientIn->hostname);
	cxa_ioStream_writeLine(ios, "Content-Type: application/json");

	cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_CONNECTED_CALC_USER_BODY);
}


static void stateCb_genUserHeaders_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_network_httpClient_t *const netClientIn = (cxa_network_httpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	cxa_ioStream_t* ios = cxa_network_tcpClient_getIoStream(netClientIn->tcpClient);

	cxa_logger_debug(&netClientIn->logger, "generating user headers (if any)");
	bool askAgain = (netClientIn->cbs.genHeaders != NULL) ?
					 netClientIn->cbs.genHeaders(netClientIn, ios, netClientIn->cbs.userVar) :
					 false;

	if( !askAgain )
	{
		cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_CONNECTED_GEN_USER_BODY);
	}
}


static void stateCb_calcUserBody_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_network_httpClient_t *const netClientIn = (cxa_network_httpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	cxa_logger_debug(&netClientIn->logger, "calculating user body size (1st pass)");

	// make our body generation stream null so we _only_ count the size of the generated body (1st pass)
	cxa_ioStream_nullablePassthrough_setNullableStream(&netClientIn->ios_bodyGeneration, NULL);
	cxa_ioStream_nullablePassthrough_resetNumByesWritten(&netClientIn->ios_bodyGeneration);
}


static void stateCb_genUserBody_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_network_httpClient_t *const netClientIn = (cxa_network_httpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	cxa_logger_debug(&netClientIn->logger, "generating user body size (2nd pass)");

	cxa_ioStream_t* ios = cxa_network_tcpClient_getIoStream(netClientIn->tcpClient);

	// make our body generation stream nonnull so we actually generate and send the body
	cxa_ioStream_nullablePassthrough_setNullableStream(&netClientIn->ios_bodyGeneration, ios);
	cxa_ioStream_nullablePassthrough_resetNumByesWritten(&netClientIn->ios_bodyGeneration);

	// need to send our end-of-header
	cxa_ioStream_writeString(ios, "\r\n");
}


static void stateCb_xxxUserBody_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_network_httpClient_t *const netClientIn = (cxa_network_httpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	cxa_ioStream_t* ios = cxa_ioStream_nullablePassthrough_getNonullStream(&netClientIn->ios_bodyGeneration);

	bool askAgain = (netClientIn->cbs.genBody != NULL) ?
					 netClientIn->cbs.genBody(netClientIn, ios, netClientIn->cbs.userVar) :
					 false;

	if( !askAgain )
	{
		// we're done...where we go now depends on our state
		if( cxa_stateMachine_getCurrentState(&netClientIn->stateMachine) == STATE_CONNECTED_CALC_USER_BODY )
		{
			// write our content length
			ios = cxa_network_tcpClient_getIoStream(netClientIn->tcpClient);
			size_t numBytes = cxa_ioStream_nullablePassthrough_getNumBytesWritten(&netClientIn->ios_bodyGeneration);
			cxa_logger_debug(&netClientIn->logger, "user body size is %d bytes", numBytes);
			cxa_ioStream_writeFormattedLine(ios, "Content-Length: %d", numBytes);
			cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_CONNECTED_GEN_USER_HEADERS);
		}
		else
		{
			cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_CONNECTED_PARSE_STATUS_CODE);
		}
	}
}


static void stateCb_parseStatusCode_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_network_httpClient_t *const netClientIn = (cxa_network_httpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	cxa_logger_debug(&netClientIn->logger, "waiting for status code");
	cxa_timeDiff_setStartTime_now(&netClientIn->td_receptionTimeout);

	// turn on our header parser
	cxa_protocolParser_crlf_resume(&netClientIn->headerLineParser);

	// wait for protocol parser callbacks
}


static void stateCb_xxxCheckTimeout_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_network_httpClient_t *const netClientIn = (cxa_network_httpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	if( cxa_timeDiff_isElapsed_ms(&netClientIn->td_receptionTimeout, netClientIn->timeout_ms) )
	{
		cxa_logger_warn(&netClientIn->logger, "reception timeout");
		cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_TRANSACTION_ERROR);
		return;
	}
}



static void stateCb_parseContentLength_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_network_httpClient_t *const netClientIn = (cxa_network_httpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	cxa_logger_debug(&netClientIn->logger, "waiting for content length");
	cxa_timeDiff_setStartTime_now(&netClientIn->td_receptionTimeout);

	// header parser should still be on at this point
	// wait for protocol parser callbacks
}


static void stateCb_forwardToBody_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_network_httpClient_t *const netClientIn = (cxa_network_httpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	cxa_logger_debug(&netClientIn->logger, "waiting for start of body");
	cxa_timeDiff_setStartTime_now(&netClientIn->td_receptionTimeout);

	// wait for protocol parser callbacks
}


static void stateCb_readBody_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_network_httpClient_t *const netClientIn = (cxa_network_httpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	// reset our response body buffer
	netClientIn->responseBody_currSize_bytes = 0;
	if(netClientIn->responseBodyBuffer != NULL) memset(netClientIn->responseBodyBuffer, 0, netClientIn->responseBody_maxSize_bytes);
	cxa_timeDiff_setStartTime_now(&netClientIn->td_receptionTimeout);

	cxa_logger_debug(&netClientIn->logger, "expecting body of %d bytes", netClientIn->responseContentLength_bytes);

	// make sure we have a body to receive
	if( netClientIn->responseContentLength_bytes == 0 )
	{
		cxa_logger_debug(&netClientIn->logger, "done reading body");
		cxa_stateMachine_transition(&netClientIn->stateMachine, netClientIn->keepOpen ? STATE_IDLE_CONNECTED : STATE_IDLE_DISCONNECTED);
		return;
	}
}


static void stateCb_readBody_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_network_httpClient_t *const netClientIn = (cxa_network_httpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	for( int i = 0; i < MAXNUM_RX_BYTES_PER_ITERATION; i++ )
	{
		uint8_t rxByte;
		cxa_ioStream_readStatus_t readState = cxa_ioStream_readByte(cxa_network_tcpClient_getIoStream(netClientIn->tcpClient), &rxByte);
		if( readState == CXA_IOSTREAM_READSTAT_ERROR )
		{
			cxa_logger_warn(&netClientIn->logger, "error reading body");
			cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_TRANSACTION_ERROR);
			return;
		}
		else if( readState == CXA_IOSTREAM_READSTAT_GOTDATA )
		{
			// reset our reception timeout
			cxa_timeDiff_setStartTime_now(&netClientIn->td_receptionTimeout);

			// store to our buffer if we have one
			if( netClientIn->responseBodyBuffer != NULL )
			{
				netClientIn->responseBodyBuffer[netClientIn->responseBody_currSize_bytes] = rxByte;
			}
			netClientIn->responseBody_currSize_bytes++;

			// see if we're done
			if( netClientIn->responseBody_currSize_bytes == netClientIn->responseContentLength_bytes )
			{
				// we're done!
				cxa_logger_debug(&netClientIn->logger, "done reading body (%d bytes) %d", netClientIn->responseBody_currSize_bytes);

				// null term our body for ease-of-use
				if( netClientIn->responseBodyBuffer != NULL ) netClientIn->responseBodyBuffer[netClientIn->responseBody_currSize_bytes] = '\0';

				// call our callback (if any)
				if( netClientIn->cbs.postComplete != NULL )
				{
					netClientIn->cbs.postComplete(netClientIn, true, netClientIn->responseStatusCode, (char*)netClientIn->responseBodyBuffer, netClientIn->responseBody_currSize_bytes, netClientIn->cbs.userVar);
				}

				// move on
				cxa_stateMachine_transition(&netClientIn->stateMachine, netClientIn->keepOpen ? STATE_IDLE_CONNECTED : STATE_IDLE_DISCONNECTED);
				return;
			}
			// if we made it here, we've got more bytes to receive

			// make sure we won't overflow (-1 is for null term)
			if( (netClientIn->responseBodyBuffer != NULL) &&
				(netClientIn->responseBody_currSize_bytes >= (netClientIn->responseBody_maxSize_bytes-1)) )
			{
				cxa_logger_warn(&netClientIn->logger, "body too big for response buffer");
				cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_TRANSACTION_ERROR);
				return;
			}
		}

		// check our body reception timeout
		if( cxa_timeDiff_isElapsed_ms(&netClientIn->td_receptionTimeout, netClientIn->timeout_ms) )
		{
			cxa_logger_warn(&netClientIn->logger, "body reception timeout");
			cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_TRANSACTION_ERROR);
			return;
		}
	}
}


static void stateCb_waitDisconnect_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_network_httpClient_t *const netClientIn = (cxa_network_httpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	cxa_logger_debug(&netClientIn->logger, "waiting for disconnect");

	// disconnect if we're connected
	if( cxa_network_tcpClient_isConnected(netClientIn->tcpClient) ) cxa_network_tcpClient_disconnect(netClientIn->tcpClient);
}


static void stateCb_waitDisconnect_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_network_httpClient_t *const netClientIn = (cxa_network_httpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	// transition can happen here OR in tcpClient callback
	if( cxa_network_tcpClient_isConnected(netClientIn->tcpClient) )
	{
		cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_IDLE_DISCONNECTED);
	}
}


static void stateCb_transactionError_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_network_httpClient_t *const netClientIn = (cxa_network_httpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	if( netClientIn->cbs.postComplete != NULL )
	{
		netClientIn->cbs.postComplete(netClientIn, false, 0, NULL, 0, netClientIn->cbs.userVar);
	}
}


static void cb_tcpClient_onConnect(cxa_network_tcpClient_t *const superIn, void* userVarIn)
{
	cxa_network_httpClient_t *const netClientIn = (cxa_network_httpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_CONNECTED_SEND_DEFAULT_HEADERS);
}


static void cb_tcpClient_onConnectFail(cxa_network_tcpClient_t *const tcpClientIn, void* userVarIn)
{
	cxa_network_httpClient_t *const netClientIn = (cxa_network_httpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	cxa_logger_debug(&netClientIn->logger, "connection failed");
	cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_TRANSACTION_ERROR);
}


static void cb_tcpClient_onDisconnect(cxa_network_tcpClient_t *const superIn, void* userVarIn)
{
	cxa_network_httpClient_t *const netClientIn = (cxa_network_httpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	switch( cxa_stateMachine_getCurrentState(&netClientIn->stateMachine) )
	{
		case STATE_IDLE_DISCONNECTED:
			// do nothing
			break;

		case STATE_WAIT_DISCONNECT:
		case STATE_TRANSACTION_ERROR:
		case STATE_IDLE_CONNECTED:
			cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_IDLE_DISCONNECTED);
			break;

		case STATE_CONNECTING:
		case STATE_CONNECTED_SEND_DEFAULT_HEADERS:
		case STATE_CONNECTED_CALC_USER_BODY:
		case STATE_CONNECTED_GEN_USER_HEADERS:
		case STATE_CONNECTED_GEN_USER_BODY:
		case STATE_CONNECTED_PARSE_STATUS_CODE:
		case STATE_CONNECTED_PARSE_CONTENT_LENGTH:
		case STATE_CONNECTED_FORWARD_TO_BODY:
		case STATE_CONNECTED_READ_BODY:
			cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_TRANSACTION_ERROR);
			break;
	}
}


static void cb_headerParser_onIoException(void *const userVarIn)
{
	cxa_network_httpClient_t *const netClientIn = (cxa_network_httpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	cxa_logger_warn(&netClientIn->logger, "ioException");
	cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_TRANSACTION_ERROR);
}


static void cb_headerParser_onReceptionTimeout(cxa_fixedByteBuffer_t *const incompletePacketIn, void *const userVarIn)
{
	cxa_network_httpClient_t *const netClientIn = (cxa_network_httpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	cxa_logger_warn(&netClientIn->logger, "reception timeout");
//	cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_TRANSACTION_ERROR);
}


static void cb_headerParser_onPacketReceived(cxa_fixedByteBuffer_t *const packetIn, void *const userVarIn)
{
	cxa_network_httpClient_t *const netClientIn = (cxa_network_httpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	state_t currState = cxa_stateMachine_getCurrentState(&netClientIn->stateMachine);
	char *const currLine = (char *const)cxa_fixedByteBuffer_get_pointerToIndex(packetIn, 0);

	cxa_logger_trace(&netClientIn->logger, "rx header: '%s'", currLine);

	if( currState == STATE_CONNECTED_PARSE_STATUS_CODE )
	{
		// we've received the line that should contain the status code...now try to parse the status code
		uint16_t statusCode;
		if( parseStatusCodeFromString(currLine, &statusCode) )
		{
			netClientIn->responseStatusCode = statusCode;
			cxa_logger_debug(&netClientIn->logger, "got status code: %d", netClientIn->responseStatusCode);
			cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_CONNECTED_PARSE_CONTENT_LENGTH);
			return;
		}
		else
		{
			// status code MUST be the first line received from the server...
			cxa_logger_warn(&netClientIn->logger, "invalid status line received");
			cxa_protocolParser_crlf_pause(&netClientIn->headerLineParser);
			cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_TRANSACTION_ERROR);
			return;
		}
	}
	else if( currState == STATE_CONNECTED_PARSE_CONTENT_LENGTH )
	{
		// always look for end of headers
		if( strlen(currLine) == 0 )
		{
			cxa_logger_warn(&netClientIn->logger, "end of headers before content length received");
			cxa_protocolParser_crlf_pause(&netClientIn->headerLineParser);
			cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_TRANSACTION_ERROR);
			return;
		}

		// see if we can parse the content length from this line...
		size_t contentLength_bytes;
		if( parseContentLengthFromString(currLine, &contentLength_bytes) )
		{
			netClientIn->responseContentLength_bytes = contentLength_bytes;
			cxa_logger_debug(&netClientIn->logger, "expecting %d bytes", netClientIn->responseContentLength_bytes);
			cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_CONNECTED_FORWARD_TO_BODY);
			return;
		}
	}
	else if( currState == STATE_CONNECTED_FORWARD_TO_BODY )
	{
		if( strlen(currLine) == 0 )
		{
			// looking for crlf line
			cxa_protocolParser_crlf_pause(&netClientIn->headerLineParser);
			cxa_stateMachine_transition(&netClientIn->stateMachine, STATE_CONNECTED_READ_BODY);
			return;
		}
	}
}
