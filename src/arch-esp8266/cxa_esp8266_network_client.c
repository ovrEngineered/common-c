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

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define DNS_LOOKUP_TIMEOUT_MS					5000
#define IPADDR_TO_STRINGARG(ip_uint32In)		((ip_uint32In >> 24) & 0x000000FF), ((ip_uint32In >> 16) & 0x000000FF), ((ip_uint32In >> 8) & 0x000000FF), ((ip_uint32In >> 0) & 0x000000FF)


// ******** local type definitions ********
typedef enum
{
	CONN_STATE_IDLE,
	CONN_STATE_DNS_LOOKUP,
	CONN_STATE_CONNECTING,
}connState_t;


// ******** local function prototypes ********
static void stateCb_idle_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_dnsLookup_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_dnsLookup_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_connecting_state(cxa_stateMachine_t *const smIn, void *userVarIn);

static bool cb_connectToHost(cxa_network_client_t *const superIn, char *const hostNameIn, uint16_t portNumIn, uint32_t timeout_msIn);
static void cb_disconnectFromHost(cxa_network_client_t *const superIn);
static void cb_dnsFound(const char *name, ip_addr_t *ipaddr, void *callback_arg);


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
	switch( espconn_gethostbyname(&netClientIn->espconn, netClientIn->targetHostName, &netClientIn->ip, cb_dnsFound) )
	{
		case ESPCONN_OK:
			// hostname is an IP or result is already cached...skip to next state
			cxa_logger_debug(&netClientIn->super.logger, "dns '%s' -> %d.%d.%d.%d", netClientIn->targetHostName, IPADDR_TO_STRINGARG(netClientIn->ip.addr));
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
}


static void stateCb_connecting_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{

}


static bool cb_connectToHost(cxa_network_client_t *const superIn, char *const hostNameIn, uint16_t portNumIn, uint32_t timeout_msIn)
{
	cxa_esp8266_network_client_t* netClientIn = (cxa_esp8266_network_client_t*)superIn;
	cxa_assert(netClientIn);

	// copy our target hostname to our local buffer
	strlcpy(netClientIn->targetHostName, hostNameIn, sizeof(netClientIn->targetHostName));
	cxa_logger_info(&netClientIn->super.logger, "connect requested to %s", netClientIn->targetHostName);

	cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_DNS_LOOKUP);

	return true;
}


static void cb_disconnectFromHost(cxa_network_client_t *const superIn)
{
	cxa_esp8266_network_client_t* netClientIn = (cxa_esp8266_network_client_t*)superIn;
	cxa_assert(netClientIn);
}


static void cb_dnsFound(const char *name, ip_addr_t *ipaddr, void *callback_arg)
{
	// find our network client
	struct espconn *conn = (struct espconn*)callback_arg;
	cxa_esp8266_network_client_t* netClientIn = cxa_esp8266_network_clientFactory_getClientByEspConn(conn);
	cxa_assert(netClientIn);

	cxa_logger_debug(&netClientIn->super.logger, "dns '%s' -> %d.%d.%d.%d", netClientIn->targetHostName, IPADDR_TO_STRINGARG(netClientIn->ip.addr));
	cxa_stateMachine_transition(&netClientIn->stateMachine, CONN_STATE_CONNECTING);
	return;
}
