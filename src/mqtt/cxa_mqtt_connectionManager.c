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
 */
#include <cxa_mqtt_connectionManager.h>


// ******** includes ********
#include <stdlib.h>
#include <string.h>
#include <cxa_assert.h>
#include <cxa_esp8266_network_factory.h>
#include <cxa_esp8266_wifiManager.h>
#include <cxa_led.h>
#define CXA_LOG_LEVEL					CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>
#include <cxa_stateMachine.h>
#include <cxa_timeBase.h>
#include <cxa_uniqueId.h>

#include <cxa_config.h>


// ******** local macro definitions ********
#define SSID							"BeerSmart"
#define KEY								"pineapple14"

//#define MQTT_SERVER						"m11.cloudmqtt.com"
//#define MQTT_SERVER_PORTNUM				13164

#define MQTT_SERVER						"m10.cloudmqtt.com"
#define MQTT_SERVER_PORTNUM				17754

#define MQTT_SERVER_USERNAME			"arsinio"
#define MQTT_SERVER_PASSWORD			"tmpPasswd"

#define BLINKPERIODMS_ON_CONFIG			100
#define BLINKPERIODMS_OFF_CONFIG		500
#define BLINKPERIODMS_ON_ASSOC			500
#define BLINKPERIODMS_OFF_ASSOC			500
#define BLINKPERIODMS_ON_CONNECTING		100
#define BLINKPERIODMS_OFF_CONNECTING	100
#define BLINKPERIODMS_ON_CONNECTED		5000
#define BLINKPERIODMS_OFF_CONNECTED		100


// ******** local type definitions ********
typedef enum
{
	STATE_ASSOCIATING,
	STATE_CONNECTING,
	STATE_CONNECTED,
	STATE_CONNECT_STANDOFF,
	STATE_CONFIG_MODE
}state_t;


// ******** local function prototypes ********
static void wifiManCb_associated(const char *const ssidIn, void* userVarIn);
static void wifiManCb_lostAssociation(const char *const ssidIn, void* userVarIn);
static void wifiManCb_configMode_enter(void* userVarIn);

static void mqttClientCb_onConnect(cxa_mqtt_client_t *const clientIn, void* userVarIn);
static void mqttClientCb_onIdle(cxa_mqtt_client_t *const clientIn, void* userVarIn);

static void stateCb_associating_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_connectStandOff_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_connectStandOff_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_configMode_enter(cxa_stateMachine_t *const smIn, void *userVarIn);


// ********  local variable declarations *********
static cxa_mqtt_client_network_t mqttClient;
static cxa_led_t led_conn;

static cxa_timeDiff_t td_connStandoff;
static uint32_t connStandoff_ms;

static cxa_stateMachine_t stateMachine;
static cxa_logger_t logger;


// ******** global function implementations ********
void cxa_mqtt_connManager_init(cxa_gpio_t *const ledConnIn)
{
	cxa_assert(ledConnIn);

	// save our references
	cxa_led_init(&led_conn, ledConnIn);

	// setup our connection standoff
	cxa_timeDiff_init(&td_connStandoff, true);
	srand(cxa_timeBase_getCount_us());

	// setup our logger
	cxa_logger_init(&logger, "connectionManager");

	// setup our state machine
	cxa_stateMachine_init(&stateMachine, "mqttConnMan");
	cxa_stateMachine_addState(&stateMachine, STATE_ASSOCIATING, "assoc", stateCb_associating_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_CONNECTING, "connecting", stateCb_connecting_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_CONNECTED, "connected", stateCb_connected_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_CONNECT_STANDOFF, "standOff", stateCb_connectStandOff_enter, stateCb_connectStandOff_state, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_CONFIG_MODE, "configMode", stateCb_configMode_enter, NULL, NULL, NULL);
	cxa_stateMachine_transition(&stateMachine, STATE_ASSOCIATING);
	cxa_stateMachine_update(&stateMachine);

	// setup our WiFi
	cxa_esp8266_wifiManager_init(NULL);
	cxa_esp8266_wifiManager_addListener(wifiManCb_configMode_enter, NULL, NULL, NULL, wifiManCb_associated, wifiManCb_lostAssociation, NULL, NULL);
	cxa_esp8266_wifiManager_addStoredNetwork(SSID, KEY);
	cxa_esp8266_wifiManager_start();

	// now setup our network connection
	cxa_esp8266_network_factory_init();

	// and our mqtt client
	cxa_mqtt_client_network_init(&mqttClient, cxa_uniqueId_getHexString());
	cxa_mqtt_client_addListener(&mqttClient.super, mqttClientCb_onIdle, mqttClientCb_onConnect, NULL, NULL, NULL);
}


cxa_mqtt_client_t* cxa_mqtt_connManager_getMqttClient(void)
{
	return &mqttClient.super;
}


void cxa_mqtt_connManager_update(void)
{
	// update everything
	cxa_led_update(&led_conn);
	cxa_esp8266_wifiManager_update();
	cxa_esp8266_network_factory_update();
	cxa_mqtt_client_update(&mqttClient.super);
	cxa_stateMachine_update(&stateMachine);
}


// ******** local function implementations ********
static void stateCb_associating_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_logger_info(&logger, "associating");
	cxa_led_blink(&led_conn, BLINKPERIODMS_ON_ASSOC,  BLINKPERIODMS_OFF_ASSOC);
}


static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_logger_info(&logger, "connecting");

	cxa_led_blink(&led_conn, BLINKPERIODMS_ON_CONNECTING, BLINKPERIODMS_OFF_CONNECTING);
	cxa_mqtt_client_network_connectToHost(&mqttClient, MQTT_SERVER, MQTT_SERVER_PORTNUM, MQTT_SERVER_USERNAME, (uint8_t*)MQTT_SERVER_PASSWORD, strlen(MQTT_SERVER_PASSWORD));
}


static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_logger_info(&logger, "connected");

	cxa_led_blink(&led_conn, BLINKPERIODMS_ON_CONNECTED, BLINKPERIODMS_OFF_CONNECTED);
}


static void stateCb_connectStandOff_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	connStandoff_ms = rand() % 1000 + 500;
	cxa_logger_info(&logger, "retry connection after %d ms", connStandoff_ms);
	cxa_timeDiff_setStartTime_now(&td_connStandoff);
}


static void stateCb_connectStandOff_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	if( cxa_timeDiff_isElapsed_ms(&td_connStandoff, connStandoff_ms) )
	{
		cxa_stateMachine_transition(&stateMachine, STATE_CONNECTING);
	}
}


static void stateCb_configMode_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_logger_info(&logger, "config mode");
	cxa_led_blink(&led_conn, BLINKPERIODMS_ON_CONFIG, BLINKPERIODMS_OFF_CONFIG);
}


static void wifiManCb_associated(const char *const ssidIn, void* userVarIn)
{
	cxa_logger_info(&logger, "associated");
	cxa_stateMachine_transition(&stateMachine, STATE_CONNECTING);
}


static void wifiManCb_lostAssociation(const char *const ssidIn, void* userVarIn)
{
	cxa_logger_warn(&logger, "lost association");

	// ensure we are disconnected regardless
	cxa_mqtt_client_disconnect(&mqttClient.super);

	cxa_stateMachine_transition(&stateMachine, STATE_ASSOCIATING);
}


static void wifiManCb_configMode_enter(void* userVarIn)
{
	cxa_stateMachine_transition(&stateMachine, STATE_CONFIG_MODE);
}


static void mqttClientCb_onConnect(cxa_mqtt_client_t *const clientIn, void* userVarIn)
{
	cxa_stateMachine_transition(&stateMachine, STATE_CONNECTED);
}


static void mqttClientCb_onIdle(cxa_mqtt_client_t *const clientIn, void* userVarIn)
{
	cxa_logger_warn(&logger, "connection failed/disconnected");
	cxa_stateMachine_transition(&stateMachine, STATE_CONNECT_STANDOFF);
}
