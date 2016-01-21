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
#include <cxa_esp8266_network_tcpServer.h>


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_esp8266_network_factory.h>

#include <user_interface.h>


#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define IPADDR_TO_STRINGARG(ip_uint32In)		((ip_uint32In >> 24) & 0x000000FF), ((ip_uint32In >> 16) & 0x000000FF), ((ip_uint32In >> 8) & 0x000000FF), ((ip_uint32In >> 0) & 0x000000FF)


// ******** local type definitions ********
typedef enum
{
	CONN_STATE_IDLE,
	CONN_STATE_LISTENING,
	CONN_STATE_CONNECTED,
}connState_t;


// ******** local function prototypes ********
static void stateCb_listening_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_connected_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);

static bool cb_listen(cxa_network_tcpServer_t *const superIn, uint16_t portNumIn);
static void cb_disconnectFromClient(cxa_network_tcpServer_t *const superIn);
static bool cb_isConnected(cxa_network_tcpServer_t *const superIn);

static void cb_espConnected(void *arg);
static void cb_espDisconnected(void *arg);
static void cb_espRx(void *arg, char *data, unsigned short len);
static void cb_espSent(void *arg);

static cxa_ioStream_readStatus_t cb_ioStream_readByte(uint8_t *const byteOut, void *const userVarIn);
static bool cb_ioStream_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_esp8266_network_tcpServer_init(cxa_esp8266_network_tcpServer_t *const tcpServerIn)
{
	cxa_assert(tcpServerIn);

	// initialize our super class
	cxa_network_tcpServer_init(&tcpServerIn->super, cb_listen, cb_disconnectFromClient, cb_isConnected);

	// setup our fifos (backing our ioStream)
	cxa_fixedFifo_initStd(&tcpServerIn->rxFifo, CXA_FF_ON_FULL_DROP, tcpServerIn->rxFifo_raw);
	cxa_fixedFifo_initStd(&tcpServerIn->txFifo, CXA_FF_ON_FULL_DROP, tcpServerIn->txFifo_raw);
	tcpServerIn->numBytesInPreviousBulkDequeue = 0;
	tcpServerIn->sendInProgress = false;

	// setup our state machine
	cxa_stateMachine_init(&tcpServerIn->stateMachine, "tcpServer");
	cxa_stateMachine_addState(&tcpServerIn->stateMachine, CONN_STATE_IDLE, "idle", NULL, NULL, NULL, (void*)tcpServerIn);
	cxa_stateMachine_addState(&tcpServerIn->stateMachine, CONN_STATE_LISTENING, "listening", stateCb_listening_enter, NULL, NULL, (void*)tcpServerIn);
	cxa_stateMachine_addState(&tcpServerIn->stateMachine, CONN_STATE_CONNECTED, "connected", stateCb_connected_enter, NULL, stateCb_connected_leave, (void*)tcpServerIn);
	cxa_stateMachine_transition(&tcpServerIn->stateMachine, CONN_STATE_IDLE);
	cxa_stateMachine_update(&tcpServerIn->stateMachine);
}


void cxa_esp8266_network_tcpServer_update(cxa_esp8266_network_tcpServer_t *const tcpServerIn)
{
	cxa_assert(tcpServerIn);

	cxa_stateMachine_update(&tcpServerIn->stateMachine);
}


// ******** local function implementations ********
static void stateCb_listening_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_esp8266_network_tcpServer_t* tcpServerIn = (cxa_esp8266_network_tcpServer_t*)userVarIn;
	cxa_assert(tcpServerIn);

	// make sure our connection has the right tcp parameters
	tcpServerIn->espconn_listen.proto.tcp = &tcpServerIn->tcp;
	tcpServerIn->espconn_listen.type = ESPCONN_TCP;
	tcpServerIn->espconn_listen.state = ESPCONN_NONE;

	// register for our callbacks
	espconn_regist_connectcb(&tcpServerIn->espconn_listen, cb_espConnected);

	espconn_accept(&tcpServerIn->espconn_listen);
}


static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_esp8266_network_tcpServer_t* tcpServerIn = (cxa_esp8266_network_tcpServer_t*)userVarIn;
	cxa_assert(tcpServerIn);

	// register for received and sent data
	espconn_regist_disconcb(tcpServerIn->espconn_client, cb_espDisconnected);
	espconn_regist_recvcb(tcpServerIn->espconn_client, cb_espRx);
	espconn_regist_sentcb(tcpServerIn->espconn_client, cb_espSent);

	// bind to our ioStream
	cxa_ioStream_bind(&tcpServerIn->super.ioStream, cb_ioStream_readByte, cb_ioStream_writeBytes, (void*)tcpServerIn);

	// notify our listeners
	cxa_array_iterate(&tcpServerIn->super.listeners, currListener, cxa_network_tcpServer_listenerEntry_t)
	{
		if( currListener == NULL ) continue;
		if( currListener->cb_onConnect != NULL ) currListener->cb_onConnect(&tcpServerIn->super, currListener->userVar);
	}
}


static void stateCb_connected_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	cxa_esp8266_network_tcpServer_t* tcpServerIn = (cxa_esp8266_network_tcpServer_t*)userVarIn;
	cxa_assert(tcpServerIn);

	// notify our listeners
	cxa_array_iterate(&tcpServerIn->super.listeners, currListener, cxa_network_tcpServer_listenerEntry_t)
	{
		if( currListener == NULL ) continue;
		if( currListener->cb_onDisconnect != NULL ) currListener->cb_onDisconnect(&tcpServerIn->super, currListener->userVar);
	}

	// unbind our ioStream
	cxa_ioStream_unbind(&tcpServerIn->super.ioStream);
}


static bool cb_listen(cxa_network_tcpServer_t *const superIn, uint16_t portNumIn)
{
	cxa_esp8266_network_tcpServer_t* tcpServerIn = (cxa_esp8266_network_tcpServer_t*)superIn;
	cxa_assert(tcpServerIn);

	// don't do anything if we're already trying
	if( cxa_stateMachine_getCurrentState(&tcpServerIn->stateMachine) != CONN_STATE_IDLE ) return false;

	// copy our port number locally
	tcpServerIn->tcp.local_port = portNumIn;
	cxa_logger_info(&tcpServerIn->super.logger, "listen requested on port %d", tcpServerIn->tcp.local_port);

	// start the sate machine transition
	cxa_stateMachine_transition(&tcpServerIn->stateMachine, CONN_STATE_LISTENING);

	return true;
}


static void cb_disconnectFromClient(cxa_network_tcpServer_t *const superIn)
{
	cxa_esp8266_network_tcpServer_t* tcpServerIn = (cxa_esp8266_network_tcpServer_t*)superIn;
	cxa_assert(tcpServerIn);

	if( (cxa_stateMachine_getCurrentState(&tcpServerIn->stateMachine) == CONN_STATE_CONNECTED)) espconn_disconnect(tcpServerIn->espconn_client);
}


static bool cb_isConnected(cxa_network_tcpServer_t *const superIn)
{
	cxa_esp8266_network_tcpServer_t* tcpServerIn = (cxa_esp8266_network_tcpServer_t*)superIn;
	cxa_assert(tcpServerIn);

	return (cxa_stateMachine_getCurrentState(&tcpServerIn->stateMachine) == CONN_STATE_CONNECTED);
}


static void cb_espConnected(void *arg)
{
	// find our network client
	struct espconn *conn = (struct espconn*)arg;
	cxa_esp8266_network_tcpServer_t* tcpServerIn = cxa_esp8266_network_factory_getTcpServerByListeningPortNum(conn->proto.tcp->local_port);
	cxa_assert(tcpServerIn);

	tcpServerIn->espconn_client = conn;

	cxa_stateMachine_transition(&tcpServerIn->stateMachine, CONN_STATE_CONNECTED);
	return;
}


static void cb_espDisconnected(void *arg)
{
	// find our network client
	struct espconn *conn = (struct espconn*)arg;
	cxa_esp8266_network_tcpServer_t* tcpServerIn = cxa_esp8266_network_factory_getTcpServerByListeningPortNum(conn->proto.tcp->local_port);
	cxa_assert(tcpServerIn);

	cxa_logger_info(&tcpServerIn->super.logger, "disconnected");
	cxa_stateMachine_transition(&tcpServerIn->stateMachine, CONN_STATE_LISTENING);
}


static void cb_espRx(void *arg, char *data, unsigned short len)
{
	// find our network client
	struct espconn *conn = (struct espconn*)arg;
	cxa_esp8266_network_tcpServer_t* tcpServerIn = cxa_esp8266_network_factory_getTcpServerByListeningPortNum(conn->proto.tcp->local_port);
	cxa_assert(tcpServerIn);

	cxa_logger_trace(&tcpServerIn->super.logger, "got %d bytes", len);

	// queue in our buffer
	bool didDropData = false;
	for( unsigned short i = 0; i < len; i++ )
	{
		if( !cxa_fixedFifo_queue(&tcpServerIn->rxFifo, (void*)&data[i]) ) didDropData = true;
	}

	if( didDropData ) cxa_logger_warn(&tcpServerIn->super.logger, "buffer overflow, data dropped");
}


static void cb_espSent(void *arg)
{
	// find our network client
	struct espconn *conn = (struct espconn*)arg;
	cxa_esp8266_network_tcpServer_t* tcpServerIn = cxa_esp8266_network_factory_getTcpServerByListeningPortNum(conn->proto.tcp->local_port);
	cxa_assert(tcpServerIn);

	// done sending
	tcpServerIn->sendInProgress = false;

	// finish the dequeue for any previously started bulk dequeues
	if( tcpServerIn->numBytesInPreviousBulkDequeue != 0 )
	{
		cxa_logger_trace(&tcpServerIn->super.logger, "bulk deueueing %d bytes", tcpServerIn->numBytesInPreviousBulkDequeue);
		cxa_fixedFifo_bulkDequeue(&tcpServerIn->txFifo, tcpServerIn->numBytesInPreviousBulkDequeue);
		tcpServerIn->numBytesInPreviousBulkDequeue = 0;
	}

	// figure out how many bytes we can send in "one shot"
	void* bytesToSend;
	size_t numContiguousBytesInFifo = cxa_fixedFifo_bulkDequeue_peek(&tcpServerIn->txFifo, &bytesToSend);
	if( numContiguousBytesInFifo > 0 )
	{
		uint16_t currNumBytesToSend = (numContiguousBytesInFifo <= UINT16_MAX) ? numContiguousBytesInFifo : UINT16_MAX;
		cxa_logger_trace(&tcpServerIn->super.logger, "transmitting %d bytes from queue", currNumBytesToSend);
		espconn_send(tcpServerIn->espconn_client, bytesToSend, currNumBytesToSend);
		tcpServerIn->numBytesInPreviousBulkDequeue = currNumBytesToSend;
	}
}


static cxa_ioStream_readStatus_t cb_ioStream_readByte(uint8_t *const byteOut, void *const userVarIn)
{
	cxa_esp8266_network_tcpServer_t* tcpServerIn = (cxa_esp8266_network_tcpServer_t*)userVarIn;
	cxa_assert(tcpServerIn);

	return cxa_fixedFifo_dequeue(&tcpServerIn->rxFifo, (void*)byteOut) ? CXA_IOSTREAM_READSTAT_GOTDATA : CXA_IOSTREAM_READSTAT_NODATA;
}


static bool cb_ioStream_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn)
{
	cxa_esp8266_network_tcpServer_t* tcpServerIn = (cxa_esp8266_network_tcpServer_t*)userVarIn;
	cxa_assert(tcpServerIn);

	if( !cxa_network_tcpServer_isConnected(&tcpServerIn->super) ) return false;

	// see if we should immediately transmit or queue
	if( !tcpServerIn->sendInProgress )
	{
		// immediately transmit...but be careful of the type size mismatch
		uint16_t currNumBytesToSend = (bufferSize_bytesIn <= UINT16_MAX) ? bufferSize_bytesIn : UINT16_MAX;
		cxa_logger_trace(&tcpServerIn->super.logger, "immediately transmitting %d bytes", currNumBytesToSend);
		espconn_send(tcpServerIn->espconn_client, buffIn, currNumBytesToSend);
		tcpServerIn->sendInProgress = true;

		// queue any remaining bytes
		size_t numBytesToQueue = bufferSize_bytesIn - currNumBytesToSend;
		cxa_logger_trace(&tcpServerIn->super.logger, "queueing %d bytes", numBytesToQueue);
		if( numBytesToQueue > 0 ) cxa_fixedFifo_bulkQueue(&tcpServerIn->txFifo, &(((uint8_t*)buffIn)[currNumBytesToSend]), numBytesToQueue);
	}
	else
	{
		cxa_logger_trace(&tcpServerIn->super.logger, "immediately queueing %d bytes", bufferSize_bytesIn);

		// we're already in the process of transmitting...esp doesn't buffer internally so we must
		cxa_fixedFifo_bulkQueue(&tcpServerIn->txFifo, buffIn, bufferSize_bytesIn);
	}

	return true;
}
