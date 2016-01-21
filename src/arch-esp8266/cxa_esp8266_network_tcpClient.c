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
#include <cxa_esp8266_network_factory.h>
#include <user_interface.h>


#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_INFO
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define IPADDR_TO_STRINGARG(ip_uint32In)		((ip_uint32In >> 24) & 0x000000FF), ((ip_uint32In >> 16) & 0x000000FF), ((ip_uint32In >> 8) & 0x000000FF), ((ip_uint32In >> 0) & 0x000000FF)


// ******** local type definitions ********
typedef enum
{
	CONN_STATE_IDLE,
	CONN_STATE_DNS_LOOKUP,
	CONN_STATE_CONNECTING,
	CONN_STATE_CONNECTED,
}connState_t;


// ******** local function prototypes ********
static void stateCb_idle_enter(cxa_stateMachine_t *const msIn, int prevStateIdIn, void *userVarIn);
static void stateCb_dnsLookup_enter(cxa_stateMachine_t *const smIn, int prevStateidIn, void *userVarIn);
static void stateCb_dnsLookup_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_connecting_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_connected_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);

static bool scm_connectToHost(cxa_network_tcpClient_t *const superIn, char *const hostNameIn, uint16_t portNumIn, bool useTlsIn, uint32_t timeout_msIn);
static void scm_disconnectFromHost(cxa_network_tcpClient_t *const superIn);
static bool scm_isConnected(cxa_network_tcpClient_t *const superIn);

static void cb_espDnsFound(const char *name, ip_addr_t *ipaddr, void *callback_arg);
static void cb_espConnected(void *arg);
static void cb_espDisconnected(void *arg);
static void cb_espRx(void *arg, char *data, unsigned short len);
static void cb_espSent(void *arg);
static void cb_espRecon(void *arg, sint8 err);

static cxa_ioStream_readStatus_t cb_ioStream_readByte(uint8_t *const byteOut, void *const userVarIn);
static bool cb_ioStream_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_esp8266_network_tcpClient_init(cxa_esp8266_network_tcpClient_t *const netClientIn)
{
	cxa_assert(netClientIn);

	// initialize our super class
	cxa_network_tcpClient_init(&netClientIn->super, scm_connectToHost, scm_disconnectFromHost, scm_isConnected);

	// setup our fifos (backing our ioStream)
	cxa_fixedFifo_initStd(&netClientIn->rxFifo, CXA_FF_ON_FULL_DROP, netClientIn->rxFifo_raw);
	cxa_fixedFifo_initStd(&netClientIn->txFifo, CXA_FF_ON_FULL_DROP, netClientIn->txFifo_raw);
	netClientIn->numBytesInPreviousBulkDequeue = 0;
	netClientIn->sendInProgress = false;

	// setup our state machine
	cxa_stateMachine_init(&netClientIn->stateMachine, "netClient");
	cxa_stateMachine_addState(&netClientIn->stateMachine, CONN_STATE_IDLE, "idle", stateCb_idle_enter, NULL, NULL, (void*)netClientIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, CONN_STATE_DNS_LOOKUP, "dnsLookup", stateCb_dnsLookup_enter, stateCb_dnsLookup_state, NULL, (void*)netClientIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, CONN_STATE_CONNECTING, "connecting", stateCb_connecting_enter, stateCb_connecting_state, NULL, (void*)netClientIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, CONN_STATE_CONNECTED, "connected", stateCb_connected_enter, NULL, stateCb_connected_leave, (void*)netClientIn);
	cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_IDLE);
	cxa_stateMachine_update(&netClientIn->stateMachine);
}


void cxa_esp8266_network_tcpClient_update(cxa_esp8266_network_tcpClient_t *const netClientIn)
{
	cxa_assert(netClientIn);

	cxa_stateMachine_update(&netClientIn->stateMachine);
}


// ******** local function implementations ********
static void stateCb_idle_enter(cxa_stateMachine_t *const msIn, int prevStateIdIn, void *userVarIn)
{
	cxa_esp8266_network_tcpClient_t* netClientIn = (cxa_esp8266_network_tcpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	cxa_array_iterate(&netClientIn->super.listeners, currListener, cxa_network_tcpClient_listenerEntry_t)
	{
		if( currListener == NULL ) continue;

		switch( prevStateIdIn )
		{
			case CONN_STATE_DNS_LOOKUP:
			case CONN_STATE_CONNECTING:
				if( currListener->cb_onConnectFail != NULL ) currListener->cb_onConnectFail(&netClientIn->super, currListener->userVar);
				break;

			case CONN_STATE_CONNECTED:
				if( currListener->cb_onDisconnect != NULL ) currListener->cb_onDisconnect(&netClientIn->super, currListener->userVar);
				break;

			default:
				break;
		}
	}
}


static void stateCb_dnsLookup_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_esp8266_network_tcpClient_t* netClientIn = (cxa_esp8266_network_tcpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	// try to get the ip address to which we'll be connecting
	switch( espconn_gethostbyname(&netClientIn->espconn, netClientIn->targetHostName, &netClientIn->ip, cb_espDnsFound) )
	{
		case ESPCONN_OK:
			// hostname is an IP or result is already cached...skip to next state
			cxa_logger_debug(&netClientIn->super.logger, "dnsCache '%s' -> %d.%d.%d.%d", netClientIn->targetHostName, IPADDR_TO_STRINGARG(netClientIn->ip.addr));
			cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_CONNECTING);
			return;
			break;

		case ESPCONN_INPROGRESS:
			// queued to send to DNS server...nothing to do here
			break;

		default:
			// error
			cxa_logger_warn(&netClientIn->super.logger, "dns lookup error");

			cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_IDLE);
			return;
			break;
	}
}


static void stateCb_dnsLookup_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_esp8266_network_tcpClient_t* netClientIn = (cxa_esp8266_network_tcpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	if( cxa_timeDiff_isElapsed_ms(&netClientIn->super.td_genPurp, netClientIn->connectTimeout_ms) )
	{
		cxa_logger_warn(&netClientIn->super.logger, "dns lookup timed out");

		cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_IDLE);
		return;
	}
}


static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_esp8266_network_tcpClient_t* netClientIn = (cxa_esp8266_network_tcpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	// setup the local information about the connection
	netClientIn->tcp.local_port = espconn_port();
	struct ip_info ipconfig;
	wifi_get_ip_info(STATION_IF, &ipconfig);
	memcpy(netClientIn->tcp.local_ip, &ipconfig.ip, sizeof(netClientIn->tcp.local_ip));

	// setup the remote information about the connection
	memcpy(netClientIn->tcp.remote_ip, &netClientIn->ip.addr, sizeof(netClientIn->tcp.remote_ip));

	// make sure our connection has the right tcp parameters
	netClientIn->espconn.proto.tcp = &netClientIn->tcp;
	netClientIn->espconn.type = ESPCONN_TCP;
	netClientIn->espconn.state = ESPCONN_NONE;

	// register our connection callbacks
	espconn_regist_connectcb(&netClientIn->espconn, cb_espConnected);
	espconn_regist_disconcb(&netClientIn->espconn, cb_espDisconnected);
	espconn_regist_reconcb(&netClientIn->espconn, cb_espRecon);

	// actually connect
	if( netClientIn->useTls ) espconn_secure_connect(&netClientIn->espconn);
	else espconn_connect(&netClientIn->espconn);
}


static void stateCb_connecting_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_esp8266_network_tcpClient_t* netClientIn = (cxa_esp8266_network_tcpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	if( cxa_timeDiff_isElapsed_ms(&netClientIn->super.td_genPurp, netClientIn->connectTimeout_ms) )
	{
		cxa_logger_warn(&netClientIn->super.logger, "connect timeout");

		cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_IDLE);
		return;
	}
}


static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_esp8266_network_tcpClient_t* netClientIn = (cxa_esp8266_network_tcpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	cxa_logger_info(&netClientIn->super.logger, "connected");

	// register for received and sent data
	espconn_regist_recvcb(&netClientIn->espconn, cb_espRx);
	espconn_regist_sentcb(&netClientIn->espconn, cb_espSent);

	// reset our queue
	netClientIn->sendInProgress = false;
	cxa_fixedFifo_clear(&netClientIn->rxFifo);
	cxa_fixedFifo_clear(&netClientIn->txFifo);

	// bind to our ioStream
	cxa_ioStream_bind(&netClientIn->super.ioStream, cb_ioStream_readByte, cb_ioStream_writeBytes, (void*)netClientIn);

	// notify our listeners
	cxa_array_iterate(&netClientIn->super.listeners, currListener, cxa_network_tcpClient_listenerEntry_t)
	{
		if( currListener == NULL ) continue;
		if( currListener->cb_onConnect != NULL ) currListener->cb_onConnect(&netClientIn->super, currListener->userVar);
	}
}


static void stateCb_connected_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	cxa_esp8266_network_tcpClient_t* netClientIn = (cxa_esp8266_network_tcpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_IDLE);

	// unbind our ioStream
	cxa_ioStream_unbind(&netClientIn->super.ioStream);
}


static bool scm_connectToHost(cxa_network_tcpClient_t *const superIn, char *const hostNameIn, uint16_t portNumIn, bool useTlsIn, uint32_t timeout_msIn)
{
	cxa_esp8266_network_tcpClient_t* netClientIn = (cxa_esp8266_network_tcpClient_t*)superIn;
	cxa_assert(netClientIn);

	// don't do anything if we're already trying
	if( cxa_stateMachine_getCurrentState(&netClientIn->stateMachine) != CONN_STATE_IDLE ) return false;

	// copy our target hostname to our local buffer
	strlcpy(netClientIn->targetHostName, hostNameIn, sizeof(netClientIn->targetHostName));
	netClientIn->tcp.remote_port = portNumIn;
	netClientIn->connectTimeout_ms = timeout_msIn;
	cxa_logger_info(&netClientIn->super.logger, "connect requested to %s:%d", netClientIn->targetHostName, netClientIn->tcp.remote_port);

	netClientIn->useTls = useTlsIn;

	// start our timeout
	cxa_timeDiff_setStartTime_now(&netClientIn->super.td_genPurp);

	// start the sate machine transition
	cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_DNS_LOOKUP);

	return true;
}


static void scm_disconnectFromHost(cxa_network_tcpClient_t *const superIn)
{
	cxa_esp8266_network_tcpClient_t* netClientIn = (cxa_esp8266_network_tcpClient_t*)superIn;
	cxa_assert(netClientIn);

	cxa_logger_info(&netClientIn->super.logger, "disconnect requested");

	// disconnect if we need to
	if( (cxa_stateMachine_getCurrentState(&netClientIn->stateMachine) == CONN_STATE_CONNECTED) )
	{
		if( netClientIn->useTls ) espconn_secure_disconnect(&netClientIn->espconn);
		else espconn_disconnect(&netClientIn->espconn);
	}
	else
	{
		// mainly for the DNS Lookup state
		cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_IDLE);
		cxa_stateMachine_update(&netClientIn->stateMachine);
	}
}


static bool scm_isConnected(cxa_network_tcpClient_t *const superIn)
{
	cxa_esp8266_network_tcpClient_t* netClientIn = (cxa_esp8266_network_tcpClient_t*)superIn;
	cxa_assert(netClientIn);

	return (cxa_stateMachine_getCurrentState(&netClientIn->stateMachine) == CONN_STATE_CONNECTED);
}


static void cb_espDnsFound(const char *name, ip_addr_t *ipaddr, void *callback_arg)
{
	// find our network client
	struct espconn *conn = (struct espconn*)callback_arg;
	cxa_esp8266_network_tcpClient_t* netClientIn = cxa_esp8266_network_factory_getTcpClientByEspConn(conn);
	cxa_assert(netClientIn);

	// make sure we are in a proper state to handle this
	if( (cxa_stateMachine_getCurrentState(&netClientIn->stateMachine) != CONN_STATE_DNS_LOOKUP) ) return;

	if( ipaddr == NULL )
	{
		cxa_logger_warn(&netClientIn->super.logger, "dns lookup failed");
		cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_IDLE);
		return;
	}

	memcpy(&netClientIn->ip, ipaddr, sizeof(netClientIn->ip));
	cxa_logger_debug(&netClientIn->super.logger, "dns '%s' -> %d.%d.%d.%d", netClientIn->targetHostName, IPADDR_TO_STRINGARG(netClientIn->ip.addr));
	cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_CONNECTING);
	return;
}


static void cb_espConnected(void *arg)
{
	// find our network client
	struct espconn *conn = (struct espconn*)arg;
	cxa_esp8266_network_tcpClient_t* netClientIn = cxa_esp8266_network_factory_getTcpClientByEspConn(conn);
	cxa_assert(netClientIn);

	if( cxa_stateMachine_getCurrentState(&netClientIn->stateMachine) == CONN_STATE_CONNECTING )
	{
		cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_CONNECTED);
	}
	return;
}


static void cb_espDisconnected(void *arg)
{
	// find our network client
	struct espconn *conn = (struct espconn*)arg;
	cxa_esp8266_network_tcpClient_t* netClientIn = cxa_esp8266_network_factory_getTcpClientByEspConn(conn);
	cxa_assert(netClientIn);

	cxa_logger_info(&netClientIn->super.logger, "disconnected");

	// make sure we're idle
	cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_IDLE);
}


static void cb_espRx(void *arg, char *data, unsigned short len)
{
	// find our network client
	struct espconn *conn = (struct espconn*)arg;
	cxa_esp8266_network_tcpClient_t* netClientIn = cxa_esp8266_network_factory_getTcpClientByEspConn(conn);
	cxa_assert(netClientIn);

	cxa_logger_trace(&netClientIn->super.logger, "got %d bytes", len);

	// queue in our buffer
	bool didDropData = false;
	for( unsigned short i = 0; i < len; i++ )
	{
		if( !cxa_fixedFifo_queue(&netClientIn->rxFifo, (void*)&data[i]) ) didDropData = true;
	}

	if( didDropData ) cxa_logger_warn(&netClientIn->super.logger, "buffer overflow, data dropped");
}


static void cb_espSent(void *arg)
{
	// find our network client
	struct espconn *conn = (struct espconn*)arg;
	cxa_esp8266_network_tcpClient_t* netClientIn = cxa_esp8266_network_factory_getTcpClientByEspConn(conn);
	cxa_assert(netClientIn);

	// done sending
	netClientIn->sendInProgress = false;

	// finish the dequeue for any previously started bulk dequeues
	if( netClientIn->numBytesInPreviousBulkDequeue != 0 )
	{
		cxa_logger_trace(&netClientIn->super.logger, "bulk deueueing %d bytes", netClientIn->numBytesInPreviousBulkDequeue);
		cxa_fixedFifo_bulkDequeue(&netClientIn->txFifo, netClientIn->numBytesInPreviousBulkDequeue);
		netClientIn->numBytesInPreviousBulkDequeue = 0;
	}

	// figure out how many bytes we can send in "one shot"
	void* bytesToSend;
	size_t numContiguousBytesInFifo = cxa_fixedFifo_bulkDequeue_peek(&netClientIn->txFifo, &bytesToSend);
	if( numContiguousBytesInFifo > 0 )
	{
		uint16_t currNumBytesToSend = (numContiguousBytesInFifo <= UINT16_MAX) ? numContiguousBytesInFifo : UINT16_MAX;
		cxa_logger_trace(&netClientIn->super.logger, "transmitting %d bytes from queue", currNumBytesToSend);
		espconn_send(&netClientIn->espconn, bytesToSend, currNumBytesToSend);
		netClientIn->numBytesInPreviousBulkDequeue = currNumBytesToSend;
	}
}


static void cb_espRecon(void *arg, sint8 err)
{
	// find our network client
	struct espconn *conn = (struct espconn*)arg;
	cxa_esp8266_network_tcpClient_t* netClientIn = cxa_esp8266_network_factory_getTcpClientByEspConn(conn);
	cxa_assert(netClientIn);

	espconn_disconnect(conn);
	cxa_logger_warn(&netClientIn->super.logger, "esp reports connection error %d, disconnected", err);

	cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_IDLE);
}


static cxa_ioStream_readStatus_t cb_ioStream_readByte(uint8_t *const byteOut, void *const userVarIn)
{
	cxa_esp8266_network_tcpClient_t* netClientIn = (cxa_esp8266_network_tcpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	return cxa_fixedFifo_dequeue(&netClientIn->rxFifo, (void*)byteOut) ? CXA_IOSTREAM_READSTAT_GOTDATA : CXA_IOSTREAM_READSTAT_NODATA;
}


static bool cb_ioStream_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn)
{
	cxa_esp8266_network_tcpClient_t* netClientIn = (cxa_esp8266_network_tcpClient_t*)userVarIn;
	cxa_assert(netClientIn);

	// handle a zero-size buffer appropriately
	if( bufferSize_bytesIn != 0 ) { cxa_assert(buffIn); }
	else { return true; }

	if( !cxa_network_tcpClient_isConnected(&netClientIn->super) ) return false;

	// see if we should immediately transmit or queue
	if( !netClientIn->sendInProgress )
	{
		// immediately transmit...but be careful of the type size mismatch
		uint16_t currNumBytesToSend = (bufferSize_bytesIn <= UINT16_MAX) ? bufferSize_bytesIn : UINT16_MAX;
		cxa_logger_trace(&netClientIn->super.logger, "immediately transmitting %d bytes", currNumBytesToSend);
		if( netClientIn->useTls ) espconn_secure_send(&netClientIn->espconn, buffIn, currNumBytesToSend);
		else espconn_send(&netClientIn->espconn, buffIn, currNumBytesToSend);
		netClientIn->sendInProgress = true;

		// queue any remaining bytes
		size_t numBytesToQueue = bufferSize_bytesIn - currNumBytesToSend;
		cxa_logger_trace(&netClientIn->super.logger, "queueing %d bytes", numBytesToQueue);
		if( numBytesToQueue > 0 ) cxa_fixedFifo_bulkQueue(&netClientIn->txFifo, &(((uint8_t*)buffIn)[currNumBytesToSend]), numBytesToQueue);
	}
	else
	{
		cxa_logger_trace(&netClientIn->super.logger, "immediately queueing %d bytes", bufferSize_bytesIn);

		// we're already in the process of transmitting...esp doesn't buffer internally so we must
		cxa_fixedFifo_bulkQueue(&netClientIn->txFifo, buffIn, bufferSize_bytesIn);
	}

	return true;
}
