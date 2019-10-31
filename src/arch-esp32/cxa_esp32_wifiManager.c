/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_network_wifiManager.h"


// ******** includes ********
#include <string.h>

#include "esp_event_loop.h"
#include "esp_smartConfig.h"
#include "esp_wifi.h"

#include <cxa_assert.h>
#include <cxa_console.h>
#include <cxa_esp32_eventManager.h>
#include <cxa_stateMachine.h>


#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define CONNECTION_TIMEOUT_MS				10000
#define MAXTIME_STOPPING_MS					5000


// ******** local type definitions ********
typedef enum
{
	STATE_IDLE,
	STATE_STARTUP,
	STATE_ASSOCIATING,
	STATE_ASSOCIATED,
	STATE_ASSOCIATION_FAILED,
	STATE_STA_STOPPING,
	STATE_PROVISIONING,
}internalState_t;


typedef enum
{
	INTTAR_MODE_DISABLED,
	INTTAR_MODE_NORMAL,
	INTTAR_MODE_PROVISIONING
}internalTargetMode_t;


typedef struct
{
	cxa_network_wifiManager_cb_t cb_idleEnter;
	cxa_network_wifiManager_cb_t cb_provisioningEnter;
	cxa_network_wifiManager_ssid_cb_t cb_associatingWithSsid;
	cxa_network_wifiManager_ssid_cb_t cb_associatedWithSsid;
	cxa_network_wifiManager_cb_t cb_unassociated;
	cxa_network_wifiManager_ssid_cb_t cb_associationWithSsidFailed;
	void *userVarIn;
}listener_t;


// ******** local function prototypes ********
static bool isStaConfigSet(void);

static void sysEventCb_sta_start(system_event_t *eventIn, void *const userVarIn);
static void sysEventCb_sta_stop(system_event_t *eventIn, void *const userVarIn);
static void sysEventCb_sta_disconnected(system_event_t *eventIn, void *const userVarIn);
static void sysEventCb_sta_gotIp(system_event_t *eventIn, void *const userVarIn);

static void stateCb_idle_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_idle_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_startup_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_associating_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_associated_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_associated_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_associationFailed_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_staStopping_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_provisioning_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);

static void espCb_smartConfig(smartconfig_status_t status, void *pdata);

static void notify_idle(void);
static void notify_provisioning(void);
static void notify_associating(void);
static void notify_associated(void);
static void notify_unassociated(void);
static void notify_associationFailed(void);

static void consoleCb_getCfg(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn);
static void consoleCb_setCfg(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn);
static void consoleCb_clearCfg(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn);
static void consoleCb_restart(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn);


// ********  local variable declarations *********
static cxa_array_t listeners;
static listener_t listeners_raw[CXA_NETWORK_WIFIMGR_MAXNUM_LISTENERS];

static internalTargetMode_t targetWifiMode = INTTAR_MODE_DISABLED;
static cxa_stateMachine_t stateMachine;

static bool didLastAssociationAttemptFail = false;

static cxa_logger_t logger;

static ip4_addr_t myIp = {.addr = 0};


// ******** global function implementations ********
void cxa_network_wifiManager_init(int threadIdIn)
{
	cxa_logger_init(&logger, "wifiMgr");

	// setup our internal arrays
	cxa_array_initStd(&listeners, listeners_raw);

	// setup our wifi sub-system
	tcpip_adapter_init();
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	cxa_assert( esp_wifi_init(&cfg) == ESP_OK );
	cxa_assert( esp_wifi_set_storage(WIFI_STORAGE_FLASH) == ESP_OK );
	cxa_assert( esp_wifi_set_mode(WIFI_MODE_STA) == ESP_OK );

	// setup our state machine
	cxa_stateMachine_init(&stateMachine, "wifiMgr", threadIdIn);
	cxa_stateMachine_addState(&stateMachine, STATE_IDLE, "idle", stateCb_idle_enter, stateCb_idle_state, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_STARTUP, "startup", stateCb_startup_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState_timed(&stateMachine, STATE_ASSOCIATING, "associating", STATE_ASSOCIATION_FAILED, CONNECTION_TIMEOUT_MS, stateCb_associating_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_ASSOCIATED, "associated", stateCb_associated_enter, NULL, stateCb_associated_leave, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_ASSOCIATION_FAILED, "assocFail", stateCb_associationFailed_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_STA_STOPPING, "staStopping", stateCb_staStopping_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_PROVISIONING, "provisioning", stateCb_provisioning_enter, NULL, NULL, NULL);
	cxa_stateMachine_setInitialState(&stateMachine, STATE_IDLE);

	// subscribe to system events
	cxa_esp32_eventManager_addListener(sysEventCb_sta_start,
									  sysEventCb_sta_stop,
									  NULL,
									  sysEventCb_sta_disconnected,
									  sysEventCb_sta_gotIp,
									  NULL,
									  NULL,
									  NULL,
									  NULL,
									  NULL,
									  NULL);

	// add our console commands
	cxa_console_addCommand("wifi.getCfg", "prints current config", NULL, 0, consoleCb_getCfg, NULL);

	cxa_console_argDescriptor_t args[2] = {
			{.dataType = CXA_STRINGUTILS_DATATYPE_STRING, .description = "ssid"},
			{.dataType = CXA_STRINGUTILS_DATATYPE_STRING, .description = "passphrase (- for open network)"}
	};
	cxa_console_addCommand("wifi.setCfg", "sets wifi config", args, 2, consoleCb_setCfg, NULL);

	cxa_console_addCommand("wifi.clearCfg", "clears current config", NULL, 0, consoleCb_clearCfg, NULL);
	cxa_console_addCommand("wifi.restart", "restarts the WiFi stateMachine", NULL, 0, consoleCb_restart, NULL);
}


void cxa_network_wifiManager_addListener(cxa_network_wifiManager_cb_t cb_idleEnterIn,
										 cxa_network_wifiManager_cb_t cb_provisioningEnterIn,
										 cxa_network_wifiManager_ssid_cb_t cb_associatingWithSsidIn,
										 cxa_network_wifiManager_ssid_cb_t cb_associatedWithSsidIn,
										 cxa_network_wifiManager_cb_t cb_unassociatedIn,
										 cxa_network_wifiManager_ssid_cb_t cb_associationWithSsidFailedIn,
										 void *userVarIn)
{
	listener_t newListener =
	{
			.cb_idleEnter = cb_idleEnterIn,
			.cb_provisioningEnter = cb_provisioningEnterIn,
			.cb_associatingWithSsid = cb_associatingWithSsidIn,
			.cb_associatedWithSsid = cb_associatedWithSsidIn,
			.cb_unassociated = cb_unassociatedIn,
			.cb_associationWithSsidFailed = cb_associationWithSsidFailedIn,
			.userVarIn = userVarIn
	};

	cxa_assert(cxa_array_append(&listeners, &newListener));
}


void cxa_network_wifiManager_start(void)
{
	switch( cxa_stateMachine_getCurrentState(&stateMachine) )
	{
		case CXA_STATE_MACHINE_STATE_UNKNOWN:
		case STATE_IDLE:
			// continue
			break;

		default:
			// don't do anything
			return;
	}

	targetWifiMode = INTTAR_MODE_NORMAL;
}


void cxa_network_wifiManager_stop(void)
{
	targetWifiMode = INTTAR_MODE_DISABLED;

	switch( cxa_stateMachine_getCurrentState(&stateMachine) )
	{
		case STATE_IDLE:
		case STATE_STARTUP:
		case STATE_STA_STOPPING:
			break;

		case STATE_ASSOCIATING:
		case STATE_ASSOCIATION_FAILED:
		case STATE_ASSOCIATED:
			cxa_stateMachine_transition(&stateMachine, STATE_STA_STOPPING);
			break;

		case STATE_PROVISIONING:
			cxa_stateMachine_transition(&stateMachine, STATE_IDLE);
			break;
	}
}


bool cxa_network_wifiManager_hasStoredCredentials(void)
{
	return isStaConfigSet();
}


void cxa_network_wifiManager_clearCredentials(void)
{
	wifi_config_t cfg;
	cfg.sta.ssid[0] = 0;
	cfg.sta.password[0] = 0;
	cfg.sta.channel = 0;
	cfg.sta.bssid_set = 0;
	cfg.sta.bssid[0] = 0;
	esp_wifi_set_config(WIFI_IF_STA, &cfg);
}


bool cxa_network_wifiManager_didLastAssociationAttemptFail(void)
{
	return didLastAssociationAttemptFail;
}


cxa_network_wifiManager_state_t cxa_network_wifiManager_getState(void)
{
	cxa_network_wifiManager_state_t retVal = CXA_NETWORK_WIFISTATE_IDLE;

	switch( cxa_stateMachine_getCurrentState(&stateMachine) )
	{
		case STATE_IDLE:
		case STATE_STARTUP:
		case STATE_STA_STOPPING:
			retVal = CXA_NETWORK_WIFISTATE_IDLE;
			break;

		case STATE_ASSOCIATING:
		case STATE_ASSOCIATION_FAILED:
			retVal = CXA_NETWORK_WIFISTATE_ASSOCIATING;
			break;

		case STATE_ASSOCIATED:
			retVal = CXA_NETWORK_WIFISTATE_ASSOCIATED;
			break;

		case STATE_PROVISIONING:
			retVal = CXA_NETWORK_WIFISTATE_PROVISIONING;
			break;
	}
	return retVal;
}


void cxa_network_wifiManager_restart(void)
{
	switch( cxa_stateMachine_getCurrentState(&stateMachine) )
	{
		case STATE_IDLE:
		case STATE_STARTUP:
			break;

		case STATE_PROVISIONING:
			cxa_stateMachine_transition(&stateMachine, STATE_STARTUP);
			break;

		default:
			cxa_stateMachine_transition(&stateMachine, STATE_STA_STOPPING);
			break;
	}
}


bool cxa_network_wifiManager_enterProvision(void)
{
	internalState_t currState = cxa_stateMachine_getCurrentState(&stateMachine);

	targetWifiMode = INTTAR_MODE_PROVISIONING;

	switch( currState )
	{
		case STATE_IDLE:
		case STATE_PROVISIONING:
			break;

		case STATE_STARTUP:
		case STATE_STA_STOPPING:
			break;

		case STATE_ASSOCIATING:
		case STATE_ASSOCIATION_FAILED:
		case STATE_ASSOCIATED:
			cxa_stateMachine_transition(&stateMachine, STATE_STA_STOPPING);
			break;
	}

	return true;
}


// ******** local function implementations ********
static bool isStaConfigSet(void)
{
	wifi_config_t wifiConfig;
	esp_wifi_get_config(WIFI_IF_STA, &wifiConfig);

	return (strlen((char*)wifiConfig.sta.ssid) > 0);
}


static void sysEventCb_sta_start(system_event_t *eventIn, void *const userVarIn)
{
	if( cxa_stateMachine_getCurrentState(&stateMachine) != STATE_STARTUP ) return;

	// we're starting up, decide whether to provision or try to associate
	if( (targetWifiMode == INTTAR_MODE_NORMAL) && isStaConfigSet() )
	{
		cxa_stateMachine_transition(&stateMachine, STATE_ASSOCIATING);
	}
	else if( targetWifiMode == INTTAR_MODE_DISABLED )
	{
		cxa_stateMachine_transition(&stateMachine, STATE_IDLE);
	}
	else
	{
		cxa_stateMachine_transition(&stateMachine, STATE_PROVISIONING);
	}
}


static void sysEventCb_sta_stop(system_event_t *eventIn, void *const userVarIn)
{
	cxa_stateMachine_transition(&stateMachine, STATE_IDLE);
}


static void sysEventCb_sta_disconnected(system_event_t *eventIn, void *const userVarIn)
{
	internalState_t currState = cxa_stateMachine_getCurrentState(&stateMachine);

	// components/esp32/include/esp_wifi_types.h -> WIFI_REASON_XXX
	cxa_logger_debug(&logger, "disconnect reason: %d", eventIn->event_info.disconnected.reason);
	if( (currState == STATE_ASSOCIATING) && (targetWifiMode != INTTAR_MODE_PROVISIONING) )
	{
		cxa_stateMachine_transition(&stateMachine, STATE_ASSOCIATION_FAILED);
	}
	else if( (currState == STATE_ASSOCIATED) && (targetWifiMode != INTTAR_MODE_PROVISIONING) )
	{
		cxa_stateMachine_transition(&stateMachine, STATE_ASSOCIATING);
	}
}


static void sysEventCb_sta_gotIp(system_event_t *eventIn, void *const userVarIn)
{
	// we're associated!!
	myIp.addr = eventIn->event_info.got_ip.ip_info.ip.addr;
	cxa_stateMachine_transition(&stateMachine, STATE_ASSOCIATED);
}


static void stateCb_idle_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "becoming idle");

	esp_wifi_stop();

	notify_idle();
}


static void stateCb_idle_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	if( targetWifiMode != INTTAR_MODE_DISABLED )
	{
		cxa_stateMachine_transition(&stateMachine, STATE_STARTUP);
	}
}


static void stateCb_startup_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "starting wifi services");

	if( targetWifiMode == INTTAR_MODE_DISABLED )
	{
		cxa_stateMachine_transition(&stateMachine, STATE_IDLE);
		return;
	}

	if( prevStateIdIn == STATE_IDLE )
	{
		esp_wifi_set_mode(WIFI_MODE_STA);
		esp_wifi_start();
	}
	else
	{
		sysEventCb_sta_start(NULL, NULL);
	}
}


static void stateCb_associating_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	wifi_config_t wifiConfig;
	esp_wifi_get_config(WIFI_IF_STA, &wifiConfig);
	cxa_logger_info(&logger, "associating with '%s'", wifiConfig.sta.ssid);

	esp_wifi_connect();

	notify_associating();
}


static void stateCb_associated_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	wifi_config_t wifiConfig;
	esp_wifi_get_config(WIFI_IF_STA, &wifiConfig);
	cxa_logger_info(&logger, "associated with '%s'", wifiConfig.sta.ssid);

	didLastAssociationAttemptFail = false;
	notify_associated();
}


static void stateCb_associated_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	wifi_config_t wifiConfig;
	esp_wifi_get_config(WIFI_IF_STA, &wifiConfig);
	cxa_logger_info(&logger, "unassociated '%s'", wifiConfig.sta.ssid);

	notify_unassociated();
}


static void stateCb_associationFailed_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	wifi_config_t wifiConfig;
	esp_wifi_get_config(WIFI_IF_STA, &wifiConfig);
	cxa_logger_info(&logger, "association with '%s' failed...retrying", wifiConfig.sta.ssid);

	didLastAssociationAttemptFail = true;
	notify_associationFailed();

	cxa_stateMachine_transition(&stateMachine, STATE_ASSOCIATING);
}


static void stateCb_staStopping_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_debug(&logger, "stopping");

	esp_wifi_disconnect();
	esp_wifi_stop();
}


static void stateCb_provisioning_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "starting provisioning");

	esp_smartconfig_set_type(SC_TYPE_ESPTOUCH);
	esp_smartconfig_start(espCb_smartConfig);

	notify_provisioning();
}


static void espCb_smartConfig(smartconfig_status_t status, void *pdata)
{
	cxa_logger_trace(&logger, "smartConfig: %d", status);

	switch( status )
	{
		case SC_STATUS_LINK:
		{
			// make a local copy
			wifi_config_t cfg;
			memcpy(&cfg.sta, pdata, sizeof(cfg.sta));

			// save our config to persistent storage
			esp_wifi_set_config(WIFI_IF_STA, &cfg);

			cxa_logger_info(&logger, "provisioned for ssid:'%s'", cfg.sta.ssid);

			// restart to apply
			targetWifiMode = INTTAR_MODE_NORMAL;
			cxa_stateMachine_transition(&stateMachine, STATE_STARTUP);
			break;
		}

		case SC_STATUS_LINK_OVER:
			cxa_logger_debug(&logger, "provision confirmed");

			// stop smart config
			esp_smartconfig_stop();
			break;

		default:
			break;
	}
}


static void notify_idle(void)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_idleEnter != NULL ) currListener->cb_idleEnter(currListener->userVarIn);
	}
}


static void notify_provisioning(void)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_provisioningEnter != NULL ) currListener->cb_provisioningEnter(currListener->userVarIn);
	}
}


static void notify_associating(void)
{
	wifi_config_t cfg;
	esp_wifi_get_config(WIFI_IF_STA, &cfg);

	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_associatingWithSsid != NULL ) currListener->cb_associatingWithSsid((char*)cfg.sta.ssid, currListener->userVarIn);
	}
}


static void notify_associated(void)
{
	wifi_config_t cfg;
	esp_wifi_get_config(WIFI_IF_STA, &cfg);

	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_associatedWithSsid != NULL ) currListener->cb_associatedWithSsid((char*)cfg.sta.ssid, currListener->userVarIn);
	}
}


static void notify_unassociated(void)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_unassociated != NULL ) currListener->cb_unassociated(currListener->userVarIn);
	}
}


static void notify_associationFailed(void)
{
	wifi_config_t cfg;
	esp_wifi_get_config(WIFI_IF_STA, &cfg);

	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_associationWithSsidFailed != NULL ) currListener->cb_associationWithSsidFailed((char*)cfg.sta.ssid, currListener->userVarIn);
	}
}


static void consoleCb_getCfg(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn)
{
	wifi_config_t cfg;
	esp_wifi_get_config(WIFI_IF_STA, &cfg);

	tcpip_adapter_ip_info_t ip;
	if( tcpip_adapter_get_ip_info(ESP_IF_WIFI_STA, &ip) != 0 )
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

	cxa_ioStream_writeFormattedLine(ioStreamIn, "ssid: %s", cfg.sta.ssid);
	cxa_ioStream_writeFormattedLine(ioStreamIn, "ip:   " IPSTR, IP2STR(&ip.ip));
	cxa_ioStream_writeFormattedLine(ioStreamIn, "sn:   " IPSTR, IP2STR(&ip.netmask));
	cxa_ioStream_writeFormattedLine(ioStreamIn, "gw:   " IPSTR, IP2STR(&ip.gw));
	cxa_ioStream_writeFormattedLine(ioStreamIn, "dns:  " IPSTR, IP2STR(&dnsInfo.ip.u_addr.ip4));
}


static void consoleCb_setCfg(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn)
{
	cxa_stringUtils_parseResult_t* ssid = cxa_array_get(argsIn, 0);
	cxa_stringUtils_parseResult_t* passphrase = cxa_array_get(argsIn, 1);
	if( (ssid == NULL) || (passphrase == NULL) )
	{
		cxa_ioStream_writeString(ioStreamIn, "error reading parameters");
		return;
	}

	bool hasPassword = !cxa_stringUtils_equals(passphrase->val_string, "-");

	wifi_config_t cfg;
	cxa_stringUtils_copy((char*)cfg.sta.ssid, ssid->val_string, sizeof(cfg.sta.ssid));
	cxa_stringUtils_copy((char*)cfg.sta.password, (hasPassword) ? passphrase->val_string : "", sizeof(cfg.sta.password));
	esp_wifi_set_config(WIFI_IF_STA, &cfg);
}


static void consoleCb_clearCfg(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn)
{
	cxa_network_wifiManager_clearCredentials();
}


static void consoleCb_restart(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn)
{
	cxa_network_wifiManager_restart();
}
