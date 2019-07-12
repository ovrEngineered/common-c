/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
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
#include <cxa_nvsManager.h>
#include <cxa_stateMachine.h>
#include <cxa_stringUtils.h>


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


#define NVSKEY_STATIC_ADDR					"eth_staticAddr"
#define NVSKEY_STATIC_NETMASK				"eth_staticNm"
#define NVSKEY_STATIC_GW						"eth_staticGw"
#define NVSKEY_STATIC_DNS					"eth_staticDns"


// ******** local type definitions ********
typedef enum
{
	STATE_IDLE,
	STATE_STARTUP,
	STATE_WAIT_FOR_LINK,
	STATE_WAIT_FOR_DHCP,
	STATE_CONFIGURE_STATIC_IP,
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
	cxa_network_ethernetManager_cb_t cb_hasAddress_staticIp;
	void *userVarIn;
}listener_t;


// ******** local function prototypes ********
static void eth_gpio_config_rmii(void);
static bool hasAutoIpAddr(void);
static bool hasStaticIpAddr(void);

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
static void stateCb_configureStaticIp_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_hasIp_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_stopping_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);

static void notifyListeners_idle(void);
static void notifyListeners_waitLink(void);
static void notifyListeners_waitDhcp(void);
static void notifyListeners_hasAddress_dhcp(void);
static void notifyListeners_hasAddress_autoIp(void);
static void notifyListeners_hasAddress_static(void);

static void consoleCb_getCfg(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn);
static void consoleCb_setStaticCfg(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn);
static void consoleCb_setDhcpCfg(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn);


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
	cxa_stateMachine_addState(&stateMachine, STATE_CONFIGURE_STATIC_IP, "configStaticIp", stateCb_configureStaticIp_enter, NULL, NULL, NULL);
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
	cxa_console_argDescriptor_t args[] = {
			{.dataType = CXA_STRINGUTILS_DATATYPE_STRING, .description = "static IP address"},
			{.dataType = CXA_STRINGUTILS_DATATYPE_STRING, .description = "subnet mask"},
			{.dataType = CXA_STRINGUTILS_DATATYPE_STRING, .description = "default gateway"},
			{.dataType = CXA_STRINGUTILS_DATATYPE_STRING, .description = "DNS server"}
	};
	cxa_console_addCommand("eth_setStaticCfg", "sets static ethernet config", args, (sizeof(args)/sizeof(*args)), consoleCb_setStaticCfg, NULL);
	cxa_console_addCommand("eth_setDhcpCfg", "sets DHCP ethernet config", NULL, 0, consoleCb_setDhcpCfg, NULL);
}


void cxa_network_ethernetManager_addListener(cxa_network_ethernetManager_cb_t cb_idleEnterIn,
											cxa_network_ethernetManager_cb_t cb_waitLinkIn,
											cxa_network_ethernetManager_cb_t cb_waitDhcpIn,
											cxa_network_ethernetManager_cb_t cb_hasAddress_dhcpIn,
											cxa_network_ethernetManager_cb_t cb_hasAddress_autoIpIn,
											cxa_network_ethernetManager_cb_t cb_hasAddress_staticIpIn,
											void *userVarIn)
{
	listener_t newListener = {
			.cb_idleEnter = cb_idleEnterIn,
			.cb_waitLink = cb_waitLinkIn,
			.cb_waitDhcp = cb_waitDhcpIn,
			.cb_hasAddress_dhcp = cb_hasAddress_dhcpIn,
			.cb_hasAddress_autoIp = cb_hasAddress_autoIpIn,
			.cb_hasAddress_staticIp = cb_hasAddress_staticIpIn
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


static bool hasStaticIpAddr(void)
{
	return cxa_nvsManager_doesKeyExist(NVSKEY_STATIC_ADDR);
}


static void sysEventCb_eth_start(system_event_t *eventIn, void *const userVarIn)
{
	if( cxa_stateMachine_getCurrentState(&stateMachine) != STATE_STARTUP ) return;

	if( hasStaticIpAddr() ) cxa_stateMachine_transition(&stateMachine, STATE_CONFIGURE_STATIC_IP);
	else cxa_stateMachine_transition(&stateMachine, STATE_WAIT_FOR_LINK);
}


static void sysEventCb_eth_stop(system_event_t *eventIn, void *const userVarIn)
{
	cxa_stateMachine_transition(&stateMachine, STATE_IDLE);
}


static void sysEventCb_eth_connected(system_event_t *eventIn, void *const userVarIn)
{
	cxa_logger_info(&logger, "link detected");

	if( hasStaticIpAddr() ) cxa_stateMachine_transition(&stateMachine, STATE_HAS_IP);
	else cxa_stateMachine_transition(&stateMachine, STATE_WAIT_FOR_DHCP);
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

	tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_ETH);

	notifyListeners_waitDhcp();
}


static void stateCb_configureStaticIp_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "configuring static address");

	// read settings from NVS
	uint32_t ip, defaultGateway, subnetMask, dnsServer;
	if( !cxa_nvsManager_get_uint32(NVSKEY_STATIC_ADDR, &ip) ||
		!cxa_nvsManager_get_uint32(NVSKEY_STATIC_NETMASK, &subnetMask) ||
		!cxa_nvsManager_get_uint32(NVSKEY_STATIC_GW, &defaultGateway) ||
		!cxa_nvsManager_get_uint32(NVSKEY_STATIC_DNS, &dnsServer) )
	{
		cxa_logger_error(&logger, "corrupt static ip NVS");
		return;
	}

	// actually do the work
	tcpip_adapter_ip_info_t ipInfo = { .ip={.addr=ip}, .gw={.addr=defaultGateway}, .netmask={.addr=subnetMask} };
	tcpip_adapter_dns_info_t dnsInfo = { .ip={.u_addr={.ip4={.addr=dnsServer}}, .type=IPADDR_TYPE_V4} };
	tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_ETH);
	tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_ETH, &ipInfo);
	tcpip_adapter_set_dns_info(TCPIP_ADAPTER_IF_ETH, TCPIP_ADAPTER_DNS_MAIN, &dnsInfo);
}


static void stateCb_hasIp_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	tcpip_adapter_ip_info_t ip;
	tcpip_adapter_get_ip_info(ESP_IF_ETH, &ip);

	if( hasStaticIpAddr() )
	{
		cxa_logger_info(&logger, "staticIp: " IPSTR, IP2STR(&ip.ip));
		notifyListeners_hasAddress_static();
	}
	else if( hasAutoIpAddr() )
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


static void notifyListeners_hasAddress_static(void)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( (currListener != NULL) && (currListener->cb_hasAddress_staticIp != NULL) ) currListener->cb_hasAddress_staticIp(currListener->userVarIn);
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

	tcpip_adapter_dns_info_t dnsInfo;
	if( tcpip_adapter_get_dns_info(TCPIP_ADAPTER_IF_ETH, TCPIP_ADAPTER_DNS_MAIN, &dnsInfo) != ESP_OK )
	{
		cxa_ioStream_writeLine(ioStreamIn, "fail");
		return;
	}

	tcpip_adapter_dhcp_status_t dhcpStat;
	if( tcpip_adapter_dhcpc_get_status(TCPIP_ADAPTER_IF_ETH, &dhcpStat) != ESP_OK ) dhcpStat = TCPIP_ADAPTER_DHCP_STOPPED;

	cxa_ioStream_writeFormattedLine(ioStreamIn, "type: %s", (dhcpStat == TCPIP_ADAPTER_DHCP_STARTED) ? "DHCP" : "static");
	cxa_ioStream_writeFormattedLine(ioStreamIn, "ip:   " IPSTR, IP2STR(&ip.ip));
	cxa_ioStream_writeFormattedLine(ioStreamIn, "sn:   " IPSTR, IP2STR(&ip.netmask));
	cxa_ioStream_writeFormattedLine(ioStreamIn, "gw:   " IPSTR, IP2STR(&ip.gw));
	cxa_ioStream_writeFormattedLine(ioStreamIn, "dns:  " IPSTR, IP2STR(&dnsInfo.ip.u_addr.ip4));
}


static void consoleCb_setStaticCfg(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn)
{
	// cxa_console has already validated the number of parameters and their types
	cxa_stringUtils_parseResult_t* ip_pr = cxa_array_get(argsIn, 0);
	cxa_stringUtils_parseResult_t* subnetMask_pr = cxa_array_get(argsIn, 1);
	cxa_stringUtils_parseResult_t* defaultGateway_pr = cxa_array_get(argsIn, 2);
	cxa_stringUtils_parseResult_t* dnsServer_pr = cxa_array_get(argsIn, 3);

	// parse our address information
	uint32_t ip;
	if( !cxa_stringUtils_ipStringToUint32(ip_pr->val_string, &ip) )
	{
		cxa_console_printErrorToIoStream(ioStreamIn, "error parsing IP address");
		return;
	}

	uint32_t subnetMask;
	if( !cxa_stringUtils_ipStringToUint32(subnetMask_pr->val_string, &subnetMask) )
	{
		cxa_console_printErrorToIoStream(ioStreamIn, "error parsing subnet mask");
		return;
	}

	uint32_t defaultGateway;
	if( !cxa_stringUtils_ipStringToUint32(defaultGateway_pr->val_string, &defaultGateway) )
	{
		cxa_console_printErrorToIoStream(ioStreamIn, "error parsing default gateway");
		return;
	}

	uint32_t dnsServer;
	if( !cxa_stringUtils_ipStringToUint32(dnsServer_pr->val_string, &dnsServer) )
	{
		cxa_console_printErrorToIoStream(ioStreamIn, "error parsing DNS server");
		return;
	}

	// save to NVS
	if( !cxa_nvsManager_set_uint32(NVSKEY_STATIC_ADDR, ip) ||
		!cxa_nvsManager_set_uint32(NVSKEY_STATIC_NETMASK, subnetMask) ||
		!cxa_nvsManager_set_uint32(NVSKEY_STATIC_GW, defaultGateway) ||
		!cxa_nvsManager_set_uint32(NVSKEY_STATIC_DNS, dnsServer) ||
		!cxa_nvsManager_commit() )
	{
		cxa_nvsManager_erase(NVSKEY_STATIC_ADDR);
		cxa_nvsManager_erase(NVSKEY_STATIC_NETMASK);
		cxa_nvsManager_erase(NVSKEY_STATIC_GW);
		cxa_nvsManager_erase(NVSKEY_STATIC_DNS);
		cxa_nvsManager_commit();

		cxa_console_printErrorToIoStream(ioStreamIn, "error committing to NVS");
		return;
	}

	// restart ethernet
	cxa_stateMachine_transition(&stateMachine, STATE_IDLE);
}


static void consoleCb_setDhcpCfg(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn)
{

	cxa_nvsManager_erase(NVSKEY_STATIC_ADDR);
	cxa_nvsManager_erase(NVSKEY_STATIC_NETMASK);
	cxa_nvsManager_erase(NVSKEY_STATIC_GW);
	cxa_nvsManager_erase(NVSKEY_STATIC_DNS);
	cxa_nvsManager_commit();

	// restart ethernet
	cxa_stateMachine_transition(&stateMachine, STATE_IDLE);
}
