/**
 * Copyright 2013 opencxa.org
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
#include "cxa_mqtt_client.h"


// ******** includes ********
#include <string.h>
#include <cxa_assert.h>
#include <cxa_stringUtils.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define KEEP_ALIVE_TIMEOUT_S			10
#define CONNACK_TIMEOUT_MS				5000
#define SUBACK_TIMEOUT_MS				5000


// ******** local type definitions ********
typedef enum
{
	MQTT_STATE_IDLE,
	MQTT_STATE_CONNECTING,
	MQTT_STATE_CONNECTED
}state_t;


// ******** local function prototypes ********
static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_connecting_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_connected_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_connected_leave(cxa_stateMachine_t *const smIn, void *userVarIn);

static void protoParseCb_onConnAck(cxa_mqtt_protocolParser_t *const mppIn, bool sessionPresentIn, cxa_mqtt_protocolParser_connAck_returnCode_t retCodeIn, void *const userVarIn);
static void protoParseCb_onPingResp(cxa_mqtt_protocolParser_t *const mppIn, void *const userVarIn);
static void protoParseCb_onSubAck(cxa_mqtt_protocolParser_t *const mppIn, uint16_t packetIdIn, cxa_mqtt_protocolParser_subAck_returnCode_t retCodeIn, void *const userVarIn);
static void protoParseCb_onPublish(cxa_mqtt_protocolParser_t *const mppIn,
									bool dupIn, cxa_mqtt_protocolParser_qosLevel_t qosIn, bool retainIn, char* topicNameIn, uint16_t packetIdIn,
									void* payloadIn, size_t payloadLen_bytesIn, void *const userVarIn);

static bool doesTopicMatchFilter(char* topicIn, char* filterIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_mqtt_client_init(cxa_mqtt_client_t *const clientIn, cxa_ioStream_t *const iosIn, cxa_timeBase_t *const timeBaseIn, char *const clientIdIn)
{
	cxa_assert(clientIn);
	cxa_assert(iosIn);
	cxa_assert(timeBaseIn);
	cxa_assert(clientIdIn);

	// save our references
	clientIn->timeBase = timeBaseIn;
	clientIn->clientId = clientIdIn;

	// setup some initial values
	clientIn->hasSentConnectPacket = false;
	clientIn->currPacketId = 0;
	cxa_timeDiff_init(&clientIn->td_timeout, timeBaseIn, true);
	cxa_timeDiff_init(&clientIn->td_sendKeepAlive, timeBaseIn, true);
	cxa_timeDiff_init(&clientIn->td_receiveKeepAlive, timeBaseIn, true);

	// setup our protocol parser
	cxa_mqtt_protocolParser_init(&clientIn->mpp, iosIn);
	cxa_mqtt_protocolParser_addListener(&clientIn->mpp, protoParseCb_onConnAck, protoParseCb_onPingResp, protoParseCb_onSubAck, protoParseCb_onPublish, (void*)clientIn);

	// setup our logger
	cxa_logger_init(&clientIn->logger, "mqttC");

	// setup our listeners array
	cxa_array_initStd(&clientIn->listeners, clientIn->listeners_raw);

	// setup our subscriptions array
	cxa_array_initStd(&clientIn->subscriptions, clientIn->subscriptions_raw);

	// setup our state machine
	cxa_stateMachine_init(&clientIn->stateMachine, "mqttClient");
	cxa_stateMachine_addState(&clientIn->stateMachine, MQTT_STATE_IDLE, "idle", NULL, NULL, NULL, (void*)clientIn);
	cxa_stateMachine_addState(&clientIn->stateMachine, MQTT_STATE_CONNECTING, "connecting", stateCb_connecting_enter, stateCb_connecting_state, NULL, (void*)clientIn);
	cxa_stateMachine_addState(&clientIn->stateMachine, MQTT_STATE_CONNECTED, "connected", stateCb_connected_enter, stateCb_connected_state, stateCb_connected_leave, (void*)clientIn);
	cxa_stateMachine_transition(&clientIn->stateMachine, MQTT_STATE_IDLE);
	cxa_stateMachine_update(&clientIn->stateMachine);
}


void cxa_mqtt_client_addListener(cxa_mqtt_client_t *const clientIn,
								 cxa_mqtt_client_cb_onConnect_t cb_onConnectIn,
								 cxa_mqtt_client_cb_onDisconnect_t cb_onDisconnectIn,
								 void *const userVarIn)
{
	cxa_assert(clientIn);

	cxa_mqtt_client_listenerEntry_t newEntry = {.cb_onConnect=cb_onConnectIn, .cb_onDisconnect=cb_onDisconnectIn, .userVar=userVarIn};
	cxa_assert( cxa_array_append(&clientIn->listeners, &newEntry) );
}


bool cxa_mqtt_client_connect(cxa_mqtt_client_t *const clientIn, char *const usernameIn, char *const passwordIn)
{
	cxa_assert(clientIn);

	if( cxa_stateMachine_getCurrentState(&clientIn->stateMachine) != MQTT_STATE_IDLE ) return false;

	// send our connect packet
	cxa_logger_info(&clientIn->logger, "connect requested");
	if( !cxa_mqtt_protocolParser_writePacket_connect(&clientIn->mpp, clientIn->clientId, usernameIn, passwordIn, true, KEEP_ALIVE_TIMEOUT_S) )
	{
		cxa_logger_warn(&clientIn->logger, "failed to send CONNECT ctrlPacket");
		return false;
	}

	cxa_stateMachine_transition(&clientIn->stateMachine, MQTT_STATE_CONNECTING);
	return true;
}


bool cxa_mqtt_client_isConnected(cxa_mqtt_client_t *const clientIn)
{
	cxa_assert(clientIn);

	return (cxa_stateMachine_getCurrentState(&clientIn->stateMachine) == MQTT_STATE_CONNECTED);
}


void cxa_mqtt_client_disconnect(cxa_mqtt_client_t *const clientIn)
{
	cxa_assert(clientIn);

	cxa_logger_info(&clientIn->logger, "disconnect requested");
	if( cxa_stateMachine_getCurrentState(&clientIn->stateMachine) == MQTT_STATE_IDLE ) return;

	cxa_stateMachine_transition(&clientIn->stateMachine, MQTT_STATE_IDLE);
}


bool cxa_mqtt_client_publish(cxa_mqtt_client_t *const clientIn, cxa_mqtt_protocolParser_qosLevel_t qosIn, bool retainIn,
							 char* topicNameIn, void *const payloadIn, size_t payloadLen_bytesIn)
{
	cxa_assert(clientIn);
	cxa_assert(topicNameIn);

	if( cxa_stateMachine_getCurrentState(&clientIn->stateMachine) != MQTT_STATE_CONNECTED ) return false;

	cxa_logger_trace(&clientIn->logger, "publish '%s' %d bytes", topicNameIn, payloadLen_bytesIn);
	return cxa_mqtt_protocolParser_writePacket_publish(&clientIn->mpp, qosIn, retainIn, topicNameIn, payloadIn, payloadLen_bytesIn);
}


void cxa_mqtt_client_subscribe(cxa_mqtt_client_t *const clientIn, char *topicFilterIn, cxa_mqtt_protocolParser_qosLevel_t qosIn, cxa_mqtt_client_cb_onPublish_t cb_onPublishIn, void* userVarIn)
{
	cxa_assert(clientIn);
	cxa_assert(topicFilterIn);
	cxa_assert(cb_onPublishIn);

	// make sure we don't have exact duplicates
	cxa_array_iterate(&clientIn->subscriptions, currSubscription, cxa_mqtt_client_subscriptionEntry_t)
	{
		if( currSubscription == NULL ) continue;
		if( cxa_stringUtils_equals(currSubscription->topicFilter, topicFilterIn) &&
			(currSubscription->qos == qosIn) &&
			(currSubscription->cb_onPublish == cb_onPublishIn) &&
			(currSubscription->userVar == userVarIn) ) return;

	}

	// create our subscription entry and add to our subscriptions
	cxa_mqtt_client_subscriptionEntry_t newEntry = {
			.state=CXA_MQTT_CLIENT_SUBSCRIPTION_STATE_UNACKNOWLEDGED,
			.packetId=clientIn->currPacketId++,
			.topicFilter=topicFilterIn,
			.qos = qosIn,
			.cb_onPublish=cb_onPublishIn,
			.userVar=userVarIn
	};
	cxa_assert( cxa_array_append(&clientIn->subscriptions, &newEntry) );

	// try to actually send our subscribe (if we're connected)
	if( cxa_stateMachine_getCurrentState(&clientIn->stateMachine) == MQTT_STATE_CONNECTED )
	{
		if( !cxa_mqtt_protocolParser_writePacket_subscribe(&clientIn->mpp, newEntry.packetId, topicFilterIn, qosIn) )
		{
			cxa_logger_warn(&clientIn->logger, "subscribe failed, subscription inoperable");
		}
	}
}


void cxa_mqtt_client_update(cxa_mqtt_client_t *const clientIn)
{
	cxa_assert(clientIn);

	cxa_stateMachine_update(&clientIn->stateMachine);
	if( cxa_stateMachine_getCurrentState(&clientIn->stateMachine) != MQTT_STATE_IDLE ) cxa_mqtt_protocolParser_update(&clientIn->mpp);
}


// ******** local function implementations ********
static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_mqtt_client_t *clientIn = (cxa_mqtt_client_t*) userVarIn;
	cxa_assert(clientIn);

	// reset our connack timeout
	cxa_timeDiff_setStartTime_now(&clientIn->td_timeout);
}


static void stateCb_connecting_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_mqtt_client_t *clientIn = (cxa_mqtt_client_t*) userVarIn;
	cxa_assert(clientIn);

	if( cxa_timeDiff_isElapsed_ms(&clientIn->td_timeout, CONNACK_TIMEOUT_MS) )
	{
		cxa_logger_warn(&clientIn->logger, "failed to receive CONNACK packet");
		cxa_stateMachine_transition(&clientIn->stateMachine, MQTT_STATE_IDLE);
		return;
	}
}


static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_mqtt_client_t *clientIn = (cxa_mqtt_client_t*) userVarIn;
	cxa_assert(clientIn);

	// start our keepalive process
	cxa_timeDiff_setStartTime_now(&clientIn->td_sendKeepAlive);
	cxa_timeDiff_setStartTime_now(&clientIn->td_receiveKeepAlive);

	// re-subscribe to our subscriptions
	cxa_array_iterate(&clientIn->subscriptions, currSubscription, cxa_mqtt_client_subscriptionEntry_t)
	{
		if( currSubscription == NULL ) continue;

		currSubscription->packetId = clientIn->currPacketId++;
		currSubscription->state = CXA_MQTT_CLIENT_SUBSCRIPTION_STATE_UNACKNOWLEDGED;

		cxa_logger_trace(&clientIn->logger, "subscribing to stored '%s'", currSubscription->topicFilter);
		if( !cxa_mqtt_protocolParser_writePacket_subscribe(&clientIn->mpp, currSubscription->packetId, currSubscription->topicFilter, currSubscription->qos) )
		{
			cxa_logger_warn(&clientIn->logger, "subscribe failed, subscription inoperable");
		}
	}

	// notify our listeners
	cxa_array_iterate(&clientIn->listeners, currListener, cxa_mqtt_client_listenerEntry_t)
	{
		if( currListener == NULL ) continue;
		if( currListener->cb_onConnect != NULL ) currListener->cb_onConnect(clientIn, currListener->userVar);
	}
}



static void stateCb_connected_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_mqtt_client_t *clientIn = (cxa_mqtt_client_t*) userVarIn;
	cxa_assert(clientIn);

	// see if we need to send a ping
	if( (KEEP_ALIVE_TIMEOUT_S != 0) && cxa_timeDiff_isElapsed_recurring_ms(&clientIn->td_sendKeepAlive, (KEEP_ALIVE_TIMEOUT_S * 1000)) )
	{
		cxa_logger_trace(&clientIn->logger, "sending PINGREQ");
		if( !cxa_mqtt_protocolParser_writePacket_pingReq(&clientIn->mpp) ) cxa_logger_warn(&clientIn->logger, "failed to tx PINGREQ");
	}

	// make sure we are receiving pings
	if( (KEEP_ALIVE_TIMEOUT_S != 0) && cxa_timeDiff_isElapsed_recurring_ms(&clientIn->td_receiveKeepAlive, (KEEP_ALIVE_TIMEOUT_S * 1000 * 2)) )
	{
		cxa_logger_warn(&clientIn->logger, "no PINGRESP, server may be unresponsive");
	}
}


static void stateCb_connected_leave(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_mqtt_client_t *clientIn = (cxa_mqtt_client_t*) userVarIn;
	cxa_assert(clientIn);

	// send disconnect message

	// notify our listeners
	cxa_array_iterate(&clientIn->listeners, currListener, cxa_mqtt_client_listenerEntry_t)
	{
		if( currListener == NULL ) continue;
		if( currListener->cb_onDisconnect != NULL ) currListener->cb_onDisconnect(clientIn, currListener->userVar);
	}
}


static void protoParseCb_onConnAck(cxa_mqtt_protocolParser_t *const mppIn, bool sessionPresentIn, cxa_mqtt_protocolParser_connAck_returnCode_t retCodeIn, void *const userVarIn)
{
	cxa_mqtt_client_t *clientIn = (cxa_mqtt_client_t*) userVarIn;
	cxa_assert(clientIn);

	if( retCodeIn == CXA_MQTT_CONNACK_RETCODE_ACCEPTED )
	{
		cxa_stateMachine_transition(&clientIn->stateMachine, MQTT_STATE_CONNECTED);
		return;
	}
	else
	{
		cxa_logger_warn(&clientIn->logger, "connection refused: %s", cxa_mqtt_protocolParser_getStringForConnAckRetCode(retCodeIn));
		cxa_stateMachine_transition(&clientIn->stateMachine, MQTT_STATE_IDLE);
	}
}


static void protoParseCb_onPingResp(cxa_mqtt_protocolParser_t *const mppIn, void *const userVarIn)
{
	cxa_mqtt_client_t *clientIn = (cxa_mqtt_client_t*) userVarIn;
	cxa_assert(clientIn);

	cxa_logger_trace(&clientIn->logger, "got PINGRESP");

	cxa_timeDiff_setStartTime_now(&clientIn->td_receiveKeepAlive);
}


static void protoParseCb_onSubAck(cxa_mqtt_protocolParser_t *const mppIn, uint16_t packetIdIn, cxa_mqtt_protocolParser_subAck_returnCode_t retCodeIn, void *const userVarIn)
{
	cxa_mqtt_client_t *clientIn = (cxa_mqtt_client_t*) userVarIn;
	cxa_assert(clientIn);

	cxa_logger_trace(&clientIn->logger, "got SUBACK for packetId %d: %d", packetIdIn, retCodeIn);

	cxa_array_iterate(&clientIn->subscriptions, currSubscription, cxa_mqtt_client_subscriptionEntry_t)
	{
		if( currSubscription == NULL ) continue;
		if( (currSubscription->state == CXA_MQTT_CLIENT_SUBSCRIPTION_STATE_UNACKNOWLEDGED) && (currSubscription->packetId == packetIdIn) )
		{
			// found our subscription...what we do now depends on whether it was successful
			if( retCodeIn == CXA_MQTT_SUBACK_RETCODE_FAILURE )
			{
				currSubscription->state = CXA_MQTT_CLIENT_SUBSCRIPTION_STATE_REFUSED;
				cxa_logger_warn(&clientIn->logger, "server refused subscription to '%s'", currSubscription->topicFilter);
			}
			else
			{
				currSubscription->state = CXA_MQTT_CLIENT_SUBSCRIPTION_STATE_ACKNOWLEDGED;
				cxa_logger_info(&clientIn->logger, "subscription to '%s' successful", currSubscription->topicFilter);
			}
		}
	}
}


static void protoParseCb_onPublish(cxa_mqtt_protocolParser_t *const mppIn,
									bool dupIn, cxa_mqtt_protocolParser_qosLevel_t qosIn, bool retainIn, char* topicNameIn, uint16_t packetIdIn,
									void* payloadIn, size_t payloadLen_bytesIn, void *const userVarIn)
{
	cxa_mqtt_client_t *clientIn = (cxa_mqtt_client_t*) userVarIn;
	cxa_assert(clientIn);

	// iterate through our subscriptions to figure out where this goes
	cxa_array_iterate(&clientIn->subscriptions, currSubscription, cxa_mqtt_client_subscriptionEntry_t)
	{
		if( currSubscription == NULL ) continue;

		if( doesTopicMatchFilter(topicNameIn, currSubscription->topicFilter) && currSubscription->cb_onPublish )
		{
			currSubscription->cb_onPublish(clientIn, topicNameIn, payloadIn, payloadLen_bytesIn,currSubscription->userVar);
		}
	}
}


// taken from: http://git.eclipse.org/c/paho/org.eclipse.paho.mqtt.embedded-c.git/tree/MQTTClient-C/src/MQTTClient.c
// assume topic filter and name is in correct format
// # can only be at end
// + and # can only be next to separator
// modified to more-fully match specification
static bool doesTopicMatchFilter(char* topicIn, char* filterIn)
{
    char* curf = filterIn;
    char* curn = topicIn;
    char* curn_end = curn + strlen(topicIn);

    while (*curf && curn < curn_end)
    {
        if (*curn == '/' && *curf != '/')
            break;
        if (*curf != '+' && *curf != '#' && *curf != *curn)
            break;
        if (*curf == '+')
        {   // skip until we meet the next separator, or end of string
            char* nextpos = curn + 1;
            while (nextpos < curn_end && *nextpos != '/')
                nextpos = ++curn + 1;
        }
        else if (*curf == '#')
        {
            curn = curn_end - 1;    // skip until end of string
        }
        curf++;
        curn++;
    };

    // special case...look for remaining /# in filter
    if( *curf == '/' ) curf++;
    if( *curf == '#' ) curf++;

    return (curn == curn_end) && (*curf == '\0');
}
