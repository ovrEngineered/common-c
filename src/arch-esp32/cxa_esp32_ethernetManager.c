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
#include "cxa_network_ethernetManager.h"


// ******** includes ********
#include <string.h>

#include "esp_event_loop.h"
#include "esp_smartConfig.h"
#include "esp_eth.h"
#include "eth_phy/phy_lan8720.h"

#include <cxa_assert.h>
#include <cxa_console.h>
#include <cxa_esp32_eventManager.h>
#include <cxa_stateMachine.h>


#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define DEFAULT_ETHERNET_PHY_CONFIG 			phy_lan8720_default_ethernet_config
#define MAXTIME_STOPPING_MS					5000

#ifndef CONFIG_PHY_ADDRESS
	#define CONFIG_PHY_ADDRESS				0
#endif

#ifndef CONFIG_PHY_SMI_MDC_PIN
	#define CONFIG_PHY_SMI_MDC_PIN			23
#endif

#ifndef CONFIG_PHY_SMI_MDIO_PIN
	#define CONFIG_PHY_SMI_MDIO_PIN 			18
#endif


// ******** local type definitions ********
typedef enum
{
	STATE_IDLE,
	STATE_STARTUP,
	STATE_WAIT_FOR_LINK,
	STATE_WAIT_FOR_DHCP,
	STATE_HAS_IP,
	STATE_ETH_STOPPING,
}internalState_t;


typedef enum
{
	INTTAR_MODE_DISABLED,
	INTTAR_MODE_NORMAL
}internalTargetMode_t;


typedef struct
{
	cxa_network_ethernetManager_cb_t cb_idleEnter;
	cxa_network_ethernetManager_cb_t cb_waitLink;
	cxa_network_ethernetManager_cb_t cb_waitDhcp;
	cxa_network_ethernetManager_cb_t cb_hasAddress_dhcp;
	cxa_network_ethernetManager_cb_t cb_hasAddress_autoIp;
	void *userVarIn;
}listener_t;


// ******** local function prototypes ********
static void eth_gpio_config_rmii(void);
static bool hasAutoIpAddr(void);

static void sysEventCb_eth_start(system_event_t *eventIn, void *const userVarIn);
static void sysEventCb_eth_stop(system_event_t *eventIn, void *const userVarIn);
static void sysEventCb_eth_connected(system_event_t *eventIn, void *const userVarIn);
static void sysEventCb_eth_disconnected(system_event_t *eventIn, void *const userVarIn);
static void sysEventCb_eth_gotIp(system_event_t *eventIn, void *const userVarIn);

static void stateCb_idle_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_idle_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_startup_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_waitForLink_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_waitForDhcp_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_hasIp_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_stopping_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);

static void notifyListeners_idle(void);
static void notifyListeners_waitLink(void);
static void notifyListeners_waitDhcp(void);
static void notifyListeners_hasAddress_dhcp(void);
static void notifyListeners_hasAddress_autoIp(void);

static void consoleCb_getCfg(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn);


// ********  local variable declarations *********
static cxa_array_t listeners;
static listener_t listeners_raw[CXA_NETWORK_ETHMGR_MAXNUM_LISTENERS];

static internalTargetMode_t targetMode = INTTAR_MODE_DISABLED;
static cxa_stateMachine_t stateMachine;

static cxa_logger_t logger;


// ******** global function implementations ********
void cxa_network_ethernetManager_init(int threadIdIn)
{
	cxa_logger_init(&logger, "ethMgr");

	// setup our internal arrays
	cxa_array_initStd(&listeners, listeners_raw);

	// setup our ethernet sub-system
	tcpip_adapter_init();
	eth_config_t config = DEFAULT_ETHERNET_PHY_CONFIG;
	config.phy_addr = CONFIG_PHY_ADDRESS;
	config.gpio_config = eth_gpio_config_rmii;
	config.tcpip_input = tcpip_adapter_eth_input;
	cxa_assert( esp_eth_init(&config) == ESP_OK );

	// setup our state machine
	cxa_stateMachine_init(&stateMachine, "ethMgr", threadIdIn);
	cxa_stateMachine_addState(&stateMachine, STATE_IDLE, "idle", stateCb_idle_enter, stateCb_idle_state, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_STARTUP, "startup", stateCb_startup_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_WAIT_FOR_LINK, "waitForLink", stateCb_waitForLink_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_WAIT_FOR_DHCP, "waitForDhcp", stateCb_waitForDhcp_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_HAS_IP, "hasIp", stateCb_hasIp_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_ETH_STOPPING, "stopping", stateCb_stopping_enter, NULL, NULL, NULL);
	cxa_stateMachine_setInitialState(&stateMachine, STATE_IDLE);

	// subscribe to system events
	cxa_esp32_eventManager_addListener(NULL,
			  	  	  	  	  	  	  NULL,
									  NULL,
									  NULL,
									  NULL,
									  sysEventCb_eth_start,
									  sysEventCb_eth_stop,
									  sysEventCb_eth_connected,
									  sysEventCb_eth_disconnected,
									  sysEventCb_eth_gotIp,
									  NULL);

	// add our console commands
	cxa_console_addCommand("eth_getCfg", "prints current config", NULL, 0, consoleCb_getCfg, NULL);
}


void cxa_network_ethernetManager_addListener(cxa_network_ethernetManager_cb_t cb_idleEnterIn,
											cxa_network_ethernetManager_cb_t cb_waitLinkIn,
											cxa_network_ethernetManager_cb_t cb_waitDhcpIn,
											cxa_network_ethernetManager_cb_t cb_hasAddress_dhcpIn,
											cxa_network_ethernetManager_cb_t cb_hasAddress_autoIpIn,
											void *userVarIn)
{
	listener_t newListener = {
			.cb_idleEnter = cb_idleEnterIn,
			.cb_waitLink = cb_waitLinkIn,
			.cb_waitDhcp = cb_waitDhcpIn,
			.cb_hasAddress_dhcp = cb_hasAddress_dhcpIn,
			.cb_hasAddress_autoIp = cb_hasAddress_autoIpIn
	};

	cxa_assert(cxa_array_append(&listeners, &newListener));
}


void cxa_network_ethernetManager_start(void)
{
	if( cxa_stateMachine_getCurrentState(&stateMachine) != STATE_IDLE ) return;

	targetMode = INTTAR_MODE_NORMAL;
}


void cxa_network_ethernetManager_stop(void)
{
	targetMode = INTTAR_MODE_DISABLED;

	switch( cxa_stateMachine_getCurrentState(&stateMachine) )
	{
		case STATE_IDLE:
		case STATE_STARTUP:
		case STATE_ETH_STOPPING:
			break;

		case STATE_WAIT_FOR_LINK:
		case STATE_WAIT_FOR_DHCP:
		case STATE_HAS_IP:
			cxa_stateMachine_transition(&stateMachine, STATE_ETH_STOPPING);
			break;
	}
}


cxa_network_ethernetManager_state_t cxa_network_ethernetManager_getState(void)
{
	cxa_network_ethernetManager_state_t retVal = CXA_NETWORK_ETHSTATE_IDLE;

	switch( cxa_stateMachine_getCurrentState(&stateMachine) )
	{
		case STATE_IDLE:
		case STATE_STARTUP:
		case STATE_ETH_STOPPING:
			retVal = CXA_NETWORK_ETHSTATE_IDLE;
			break;

		case STATE_WAIT_FOR_LINK:
			retVal = CXA_NETWORK_ETHSTATE_WAIT_LINK;
			break;

		case STATE_WAIT_FOR_DHCP:
			retVal = CXA_NETWORK_ETHSTATE_WAIT_DHCP;
			break;

		case STATE_HAS_IP:
			retVal = hasAutoIpAddr() ? CXA_NETWORK_ETHSTATE_HAS_ADDRESS_AUTOIP : CXA_NETWORK_ETHSTATE_HAS_ADDRESS_DHCP;
			break;
	}
	return retVal;
}


// ******** local function implementations ********
static void eth_gpio_config_rmii(void)
{
    // RMII data pins are fixed:
    // TXD0 = GPIO19
    // TXD1 = GPIO22
    // TX_EN = GPIO21
    // RXD0 = GPIO25
    // RXD1 = GPIO26
    // CLK == GPIO0
    phy_rmii_configure_data_interface_pins();
    // MDC is GPIO 23, MDIO is GPIO 18
    phy_rmii_smi_configure_pins(CONFIG_PHY_SMI_MDC_PIN, CONFIG_PHY_SMI_MDIO_PIN);
}


static bool hasAutoIpAddr(void)
{
	tcpip_adapter_ip_info_t ip;
	if( tcpip_adapter_get_ip_info(ESP_IF_ETH, &ip) != 0 ) return false;

	return (ip4_addr1(&ip.ip) == 169) && (ip4_addr2(&ip.ip) == 254);
}


static void sysEventCb_eth_start(system_event_t *eventIn, void *const userVarIn)
{
	if( cxa_stateMachine_getCurrentState(&stateMachine) != STATE_STARTUP ) return;

	cxa_stateMachine_transition(&stateMachine, STATE_WAIT_FOR_LINK);
}


static void sysEventCb_eth_stop(system_event_t *eventIn, void *const userVarIn)
{
	cxa_stateMachine_transition(&stateMachine, STATE_IDLE);
}


static void sysEventCb_eth_connected(system_event_t *eventIn, void *const userVarIn)
{
	cxa_logger_info(&logger, "link detected");
	cxa_stateMachine_transition(&stateMachine, STATE_WAIT_FOR_DHCP);
}


static void sysEventCb_eth_disconnected(system_event_t *eventIn, void *const userVarIn)
{
	cxa_logger_info(&logger, "link lost");
	if( targetMode != INTTAR_MODE_DISABLED) cxa_stateMachine_transition(&stateMachine, STATE_WAIT_FOR_LINK);
}


static void sysEventCb_eth_gotIp(system_event_t *eventIn, void *const userVarIn)
{
	if( targetMode != INTTAR_MODE_DISABLED) cxa_stateMachine_transition(&stateMachine, STATE_HAS_IP);
}


static void stateCb_idle_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "becoming idle");

	esp_eth_disable();

	notifyListeners_idle();
}


static void stateCb_idle_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	if( targetMode != INTTAR_MODE_DISABLED )
	{
		cxa_stateMachine_transition(&stateMachine, STATE_STARTUP);
	}
}


static void stateCb_startup_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "starting ethernet services");

	if( targetMode == INTTAR_MODE_DISABLED )
	{
		cxa_stateMachine_transition(&stateMachine, STATE_IDLE);
		return;
	}

	esp_eth_enable();
}


static void stateCb_waitForLink_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "waiting for link");

	notifyListeners_waitLink();
}


static void stateCb_waitForDhcp_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "waiting for address from dhcp");

	notifyListeners_waitDhcp();
}


static void stateCb_hasIp_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	tcpip_adapter_ip_info_t ip;
	tcpip_adapter_get_ip_info(ESP_IF_ETH, &ip);

	if( hasAutoIpAddr() )
	{
		cxa_logger_info(&logger, "autoIp: " IPSTR, IP2STR(&ip.ip));
		notifyListeners_hasAddress_autoIp();
	}
	else
	{
		cxa_logger_info(&logger, "dhcpIp: " IPSTR, IP2STR(&ip.ip));
		notifyListeners_hasAddress_dhcp();
	}
}


static void stateCb_stopping_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "stopping");

	esp_eth_disable();
}


static void notifyListeners_idle(void)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( (currListener != NULL) && (currListener->cb_idleEnter != NULL) ) currListener->cb_idleEnter(currListener->userVarIn);
	}
}


static void notifyListeners_waitLink(void)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( (currListener != NULL) && (currListener->cb_waitLink != NULL) ) currListener->cb_waitLink(currListener->userVarIn);
	}
}


static void notifyListeners_waitDhcp(void)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( (currListener != NULL) && (currListener->cb_waitDhcp != NULL) ) currListener->cb_waitDhcp(currListener->userVarIn);
	}
}


static void notifyListeners_hasAddress_dhcp(void)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( (currListener != NULL) && (currListener->cb_hasAddress_dhcp != NULL) ) currListener->cb_hasAddress_dhcp(currListener->userVarIn);
	}
}


static void notifyListeners_hasAddress_autoIp(void)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( (currListener != NULL) && (currListener->cb_hasAddress_autoIp != NULL) ) currListener->cb_hasAddress_autoIp(currListener->userVarIn);
	}
}


static void consoleCb_getCfg(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn)
{
	tcpip_adapter_ip_info_t ip;
	if( tcpip_adapter_get_ip_info(ESP_IF_ETH, &ip) != 0 )
	{
		cxa_ioStream_writeLine(ioStreamIn, "fail");
		return;
	}

	cxa_ioStream_writeFormattedLine(ioStreamIn, "ip: " IPSTR, IP2STR(&ip.ip));
	cxa_ioStream_writeFormattedLine(ioStreamIn, "sn: " IPSTR, IP2STR(&ip.netmask));
	cxa_ioStream_writeFormattedLine(ioStreamIn, "gw: " IPSTR, IP2STR(&ip.gw));
}
