/**
 * Original Source:
 * https://github.com/esp8266/Arduino.git
 *
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
#include <string.h>
#include <cxa_assert.h>
#include <cxa_esp8266_network_client.h>
#include <cxa_esp8266_network_clientFactory.h>

#include <user_interface.h>


#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define DNS_LOOKUP_TIMEOUT_MS					15000
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
static void stateCb_idle_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_dnsLookup_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_dnsLookup_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_connecting_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, void *userVarIn);

static bool cb_connectToHost(cxa_network_client_t *const superIn, char *const hostNameIn, uint16_t portNumIn, uint32_t timeout_msIn, bool autoReconnectIn);
static void cb_disconnectFromHost(cxa_network_client_t *const superIn);

static void cb_espDnsFound(const char *name, ip_addr_t *ipaddr, void *callback_arg);
static void cb_espConnected(void *arg);
static void cb_espDisconnected(void *arg);
static void cb_espRx(void *arg, char *data, unsigned short len);
static void cb_espRecon(void *arg, sint8 err);



// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_esp8266_network_client_init(cxa_esp8266_network_client_t *const netClientIn, cxa_timeBase_t* const timeBaseIn)
{
	cxa_assert(netClientIn);
	cxa_assert(timeBaseIn);

	// initialize our super class
	cxa_network_client_init(&netClientIn->super, timeBaseIn, cb_connectToHost, cb_disconnectFromHost);

	// setup our state machine
	cxa_stateMachine_init(&netClientIn->stateMachine, "netClient");
	cxa_stateMachine_addState(&netClientIn->stateMachine, CONN_STATE_IDLE, "idle", stateCb_idle_enter, NULL, NULL, (void*)netClientIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, CONN_STATE_DNS_LOOKUP, "dnsLookup", stateCb_dnsLookup_enter, stateCb_dnsLookup_state, NULL, (void*)netClientIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, CONN_STATE_CONNECTING, "connecting", stateCb_connecting_enter, stateCb_connecting_state, NULL, (void*)netClientIn);
	cxa_stateMachine_addState(&netClientIn->stateMachine, CONN_STATE_CONNECTED, "connected", stateCb_connected_enter, NULL, NULL, (void*)netClientIn);
	cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_IDLE);
}


void cxa_esp8266_network_client_update(cxa_esp8266_network_client_t *const netClientIn)
{
	cxa_assert(netClientIn);

	cxa_stateMachine_update(&netClientIn->stateMachine);
}


// ******** local function implementations ********
static void stateCb_idle_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{

}


static void stateCb_dnsLookup_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_esp8266_network_client_t* netClientIn = (cxa_esp8266_network_client_t*)userVarIn;
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

	// start our timeout
	cxa_timeDiff_setStartTime_now(&netClientIn->super.td_genPurp);
}


static void stateCb_dnsLookup_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_esp8266_network_client_t* netClientIn = (cxa_esp8266_network_client_t*)userVarIn;
	cxa_assert(netClientIn);

	if( cxa_timeDiff_isElapsed_ms(&netClientIn->super.td_genPurp, DNS_LOOKUP_TIMEOUT_MS) )
	{
		cxa_logger_warn(&netClientIn->super.logger, "dns lookup timed out");
		cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_IDLE);
		return;
	}
}


static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_esp8266_network_client_t* netClientIn = (cxa_esp8266_network_client_t*)userVarIn;
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

	// register for our callbacks
	espconn_regist_connectcb(&netClientIn->espconn, cb_espConnected);
	espconn_regist_disconcb(&netClientIn->espconn, cb_espDisconnected);
	espconn_regist_recvcb(&netClientIn->espconn, cb_espRx);
	espconn_regist_reconcb(&netClientIn->espconn, cb_espRecon);

	// actually connect
	espconn_connect(&netClientIn->espconn);

	// start our timeout
	cxa_timeDiff_setStartTime_now(&netClientIn->super.td_genPurp);
}


static void stateCb_connecting_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_esp8266_network_client_t* netClientIn = (cxa_esp8266_network_client_t*)userVarIn;
	cxa_assert(netClientIn);

	if( cxa_timeDiff_isElapsed_ms(&netClientIn->super.td_genPurp, netClientIn->connectTimeout_ms) )
	{
		cxa_logger_warn(&netClientIn->super.logger, "connect timeout, retrying");
		cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_CONNECTING);
		return;
	}
}


static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_esp8266_network_client_t* netClientIn = (cxa_esp8266_network_client_t*)userVarIn;
	cxa_assert(netClientIn);
}


static bool cb_connectToHost(cxa_network_client_t *const superIn, char *const hostNameIn, uint16_t portNumIn, uint32_t timeout_msIn, bool autoReconnectIn)
{
	cxa_esp8266_network_client_t* netClientIn = (cxa_esp8266_network_client_t*)superIn;
	cxa_assert(netClientIn);

	// copy our target hostname to our local buffer
	strlcpy(netClientIn->targetHostName, hostNameIn, sizeof(netClientIn->targetHostName));
	netClientIn->tcp.remote_port = portNumIn;
	netClientIn->connectTimeout_ms = timeout_msIn;
	netClientIn->autoReconnect = autoReconnectIn;
	cxa_logger_info(&netClientIn->super.logger, "connect requested to %s:%d", netClientIn->targetHostName, netClientIn->tcp.remote_port);

	cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_DNS_LOOKUP);

	return true;
}


static void cb_disconnectFromHost(cxa_network_client_t *const superIn)
{
	cxa_esp8266_network_client_t* netClientIn = (cxa_esp8266_network_client_t*)superIn;
	cxa_assert(netClientIn);
}


static void cb_espDnsFound(const char *name, ip_addr_t *ipaddr, void *callback_arg)
{
	// find our network client
	struct espconn *conn = (struct espconn*)callback_arg;
	cxa_esp8266_network_client_t* netClientIn = cxa_esp8266_network_clientFactory_getClientByEspConn(conn);
	cxa_assert(netClientIn);

	if( ipaddr == NULL )
	{
		cxa_logger_warn(&netClientIn->super.logger, "dns lookup failed, retrying");
		cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_DNS_LOOKUP);
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
	cxa_esp8266_network_client_t* netClientIn = cxa_esp8266_network_clientFactory_getClientByEspConn(conn);
	cxa_assert(netClientIn);

	cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_CONNECTED);
	return;
}


static void cb_espDisconnected(void *arg)
{
	// find our network client
	struct espconn *conn = (struct espconn*)arg;
	cxa_esp8266_network_client_t* netClientIn = cxa_esp8266_network_clientFactory_getClientByEspConn(conn);
	cxa_assert(netClientIn);

	if( netClientIn->autoReconnect )
	{
		cxa_logger_info(&netClientIn->super.logger, "disconnect, auto re-connecting");
		cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_CONNECTING);
		return;
	}
	else
	{
		cxa_logger_info(&netClientIn->super.logger, "disconnected");
		cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_IDLE);
		return;
	}
}


static void cb_espRx(void *arg, char *data, unsigned short len)
{

}


static void cb_espRecon(void *arg, sint8 err)
{
	// find our network client
	struct espconn *conn = (struct espconn*)arg;
	cxa_esp8266_network_client_t* netClientIn = cxa_esp8266_network_clientFactory_getClientByEspConn(conn);
	cxa_assert(netClientIn);

	cxa_logger_warn(&netClientIn->super.logger, "esp reports connection error %d", err);
	if( netClientIn->autoReconnect )
	{
		cxa_logger_info(&netClientIn->super.logger, "disconnect, auto re-connecting");
		cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_CONNECTING);
		return;
	}
	else
	{
		cxa_logger_info(&netClientIn->super.logger, "disconnected");
		cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_IDLE);
		return;
	}
}
