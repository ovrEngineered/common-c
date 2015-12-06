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
#include <cxa_mqtt_messageFactory.h>
#include <cxa_mqtt_message_connack.h>
#include <cxa_mqtt_message_connect.h>
#include <cxa_mqtt_message_pingRequest.h>
#include <cxa_mqtt_message_subscribe.h>
#include <cxa_mqtt_message_suback.h>
#include <cxa_mqtt_message_publish.h>
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

static void protoParseCb_onPacketReceived(cxa_fixedByteBuffer_t *const packetIn, void *const userVarIn);

static void handleMessage_connAck(cxa_mqtt_client_t *const clientIn, cxa_mqtt_message_t *const msgIn);
static void handleMessage_pingResp(cxa_mqtt_client_t *const clientIn, cxa_mqtt_message_t *const msgIn);
static void handleMessage_subAck(cxa_mqtt_client_t *const clientIn, cxa_mqtt_message_t *const msgIn);
static void handleMessage_publish(cxa_mqtt_client_t *const clientIn, cxa_mqtt_message_t *const msgIn);

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

	// get a message (and buffer) for our protocol parser
	cxa_mqtt_message_t* msg = cxa_mqtt_messageFactory_getFreeMessage_empty();
	cxa_assert(msg);

	// setup our protocol parser
	cxa_protocolParser_mqtt_init(&clientIn->mpp, iosIn, msg->buffer, timeBaseIn);
	cxa_protocolParser_addPacketListener(&clientIn->mpp.super, protoParseCb_onPacketReceived, (void*)clientIn);

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


bool cxa_mqtt_client_connect(cxa_mqtt_client_t *const clientIn, char *const usernameIn, uint8_t *const passwordIn, uint16_t passwordLen_bytesIn)
{
	cxa_assert(clientIn);

	if( cxa_stateMachine_getCurrentState(&clientIn->stateMachine) != MQTT_STATE_IDLE ) return false;

	// reserve/initialize/send message
	cxa_mqtt_message_t* msg = NULL;
	if( ((msg = cxa_mqtt_messageFactory_getFreeMessage_empty()) == NULL) ||
			!cxa_mqtt_message_connect_init(msg, clientIn->clientId, usernameIn, passwordIn, passwordLen_bytesIn, true, KEEP_ALIVE_TIMEOUT_S) ||
			!cxa_protocolParser_writePacket(&clientIn->mpp.super, cxa_mqtt_message_getBuffer(msg)) )
	{
		cxa_logger_warn(&clientIn->logger, "failed to reserve/initialize/send CONNECT ctrlPacket");
		if( msg != NULL ) cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
		return false;
	}
	if( msg != NULL ) cxa_mqtt_messageFactory_decrementMessageRefCount(msg);

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


bool cxa_mqtt_client_publish(cxa_mqtt_client_t *const clientIn, cxa_mqtt_qosLevel_t qosIn, bool retainIn,
							 char* topicNameIn, void *const payloadIn, size_t payloadLen_bytesIn)
{
	cxa_assert(clientIn);
	cxa_assert(topicNameIn);

	if( cxa_stateMachine_getCurrentState(&clientIn->stateMachine) != MQTT_STATE_CONNECTED ) return false;

	cxa_logger_trace(&clientIn->logger, "publish '%s' %d bytes", topicNameIn, payloadLen_bytesIn);
	//return cxa_mqtt_protocolParser_writePacket_publish(&clientIn->mpp, qosIn, retainIn, topicNameIn, payloadIn, payloadLen_bytesIn);
	return false;
}


void cxa_mqtt_client_subscribe(cxa_mqtt_client_t *const clientIn, char *topicFilterIn, cxa_mqtt_qosLevel_t qosIn, cxa_mqtt_client_cb_onPublish_t cb_onPublishIn, void* userVarIn)
{
	cxa_assert(clientIn);
	cxa_assert(topicFilterIn);
	cxa_assert(strlen(topicFilterIn) <= CXA_MQTT_CLIENT_MAXLEN_TOPICFILTER_BYTES);
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
			.qos = qosIn,
			.cb_onPublish=cb_onPublishIn,
			.userVar=userVarIn
	};
	strlcpy(newEntry.topicFilter, topicFilterIn, sizeof(newEntry.topicFilter));
	cxa_assert( cxa_array_append(&clientIn->subscriptions, &newEntry) );

	// try to actually send our subscribe (if we're connected)
	if( cxa_stateMachine_getCurrentState(&clientIn->stateMachine) == MQTT_STATE_CONNECTED )
	{
		cxa_mqtt_message_t* msg = NULL;
		if( ((msg = cxa_mqtt_messageFactory_getFreeMessage_empty()) == NULL) ||
				!cxa_mqtt_message_subscribe_init(msg, newEntry.packetId, topicFilterIn, qosIn) ||
				!cxa_protocolParser_writePacket(&clientIn->mpp.super, cxa_mqtt_message_getBuffer(msg)) )
		{
			cxa_logger_warn(&clientIn->logger, "subscribe reserve/initialize/send failed, subscription inoperable");
		}
		if( msg != NULL ) cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
	}

}


void cxa_mqtt_client_update(cxa_mqtt_client_t *const clientIn)
{
	cxa_assert(clientIn);

	cxa_stateMachine_update(&clientIn->stateMachine);
	cxa_protocolParser_mqtt_update(&clientIn->mpp);
}


void cxa_mqtt_client_internalDisconnect(cxa_mqtt_client_t *const clientIn)
{
	cxa_assert(clientIn);

	if( cxa_stateMachine_getCurrentState(&clientIn->stateMachine) == MQTT_STATE_IDLE ) return;

	cxa_stateMachine_transition(&clientIn->stateMachine, MQTT_STATE_IDLE);
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
		cxa_mqtt_message_t* msg = NULL;
		if( ((msg = cxa_mqtt_messageFactory_getFreeMessage_empty()) == NULL) ||
				!cxa_mqtt_message_subscribe_init(msg, currSubscription->packetId, currSubscription->topicFilter, currSubscription->qos) ||
				!cxa_protocolParser_writePacket(&clientIn->mpp.super, cxa_mqtt_message_getBuffer(msg)) )
		{
			cxa_logger_warn(&clientIn->logger, "subscribe reserve/initialize/send failed, subscription inoperable");
		}
		if( msg != NULL ) cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
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
		cxa_mqtt_message_t* msg = NULL;
		if( ((msg = cxa_mqtt_messageFactory_getFreeMessage_empty()) == NULL) ||
				!cxa_mqtt_message_pingRequest_init(msg) ||
				!cxa_protocolParser_writePacket(&clientIn->mpp.super, cxa_mqtt_message_getBuffer(msg)) )
		{
			cxa_logger_warn(&clientIn->logger, "failed to reserve/initialize/send PINGREQ ctrlPacket");
		}
		if( msg != NULL ) cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
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


static void protoParseCb_onPacketReceived(cxa_fixedByteBuffer_t *const packetIn, void *const userVarIn)
{
	cxa_mqtt_client_t *clientIn = (cxa_mqtt_client_t*) userVarIn;
	cxa_assert(clientIn);

	// if we're not supposed to be processing data, don't do it
	if( cxa_stateMachine_getCurrentState(&clientIn->stateMachine) == MQTT_STATE_IDLE ) return;

	cxa_mqtt_message_t* msg = cxa_mqtt_messageFactory_getMessage_byBuffer(packetIn);
	if( msg == NULL ) return;

	cxa_mqtt_message_type_t msgType = cxa_mqtt_message_getType(msg);
	switch( msgType )
	{
		case CXA_MQTT_MSGTYPE_CONNACK:
			handleMessage_connAck(clientIn, msg);
			break;

		case CXA_MQTT_MSGTYPE_PINGRESP:
			handleMessage_pingResp(clientIn, msg);
			break;

		case CXA_MQTT_MSGTYPE_SUBACK:
			handleMessage_subAck(clientIn, msg);
			break;

		case CXA_MQTT_MSGTYPE_PUBLISH:
			handleMessage_publish(clientIn, msg);
			break;

		default:
			cxa_logger_trace(&clientIn->logger, "got unknown msgType: %d", msgType);
			break;
	}
}


static void handleMessage_connAck(cxa_mqtt_client_t *const clientIn, cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(clientIn);
	cxa_assert(msgIn);

	cxa_mqtt_connAck_returnCode_t retCode = CXA_MQTT_CONNACK_RETCODE_UNKNOWN;
	if( cxa_mqtt_message_connack_getReturnCode(msgIn, &retCode ) && (retCode == CXA_MQTT_CONNACK_RETCODE_ACCEPTED) )
	{
		cxa_stateMachine_transition(&clientIn->stateMachine, MQTT_STATE_CONNECTED);
		return;
	}
	else
	{
		cxa_logger_warn(&clientIn->logger, "connection refused: %d", retCode);
		cxa_stateMachine_transition(&clientIn->stateMachine, MQTT_STATE_IDLE);
		return;
	}
}


static void handleMessage_pingResp(cxa_mqtt_client_t *const clientIn, cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(clientIn);
	cxa_assert(msgIn);

	cxa_logger_trace(&clientIn->logger, "got PINGRESP");
	cxa_timeDiff_setStartTime_now(&clientIn->td_receiveKeepAlive);
}


static void handleMessage_subAck(cxa_mqtt_client_t *const clientIn, cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(clientIn);
	cxa_assert(msgIn);

	cxa_mqtt_subAck_returnCode_t retCode;
	uint16_t packetId;
	if( cxa_mqtt_message_suback_getPacketId(msgIn, &packetId) && cxa_mqtt_message_suback_getReturnCode(msgIn, &retCode) )
	{
		cxa_logger_trace(&clientIn->logger, "got SUBACK for packetId %d: %d", packetId, retCode);

		cxa_array_iterate(&clientIn->subscriptions, currSubscription, cxa_mqtt_client_subscriptionEntry_t)
		{
			if( currSubscription == NULL ) continue;
			if( (currSubscription->state == CXA_MQTT_CLIENT_SUBSCRIPTION_STATE_UNACKNOWLEDGED) && (currSubscription->packetId == packetId) )
			{
				// found our subscription...what we do now depends on whether it was successful
				if( retCode == CXA_MQTT_SUBACK_RETCODE_FAILURE )
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
	} else cxa_logger_warn(&clientIn->logger, "malformed SUBACK");
}


static void handleMessage_publish(cxa_mqtt_client_t *const clientIn, cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(clientIn);
	cxa_assert(msgIn);

	// iterate through our subscriptions to figure out where this goes
	char *topicName;
	void *payload;
	size_t topicNameLen_bytes, payloadSize_bytes;
	if( cxa_mqtt_message_publish_getTopicName(msgIn, &topicName, &topicNameLen_bytes) && cxa_mqtt_message_publish_getPayload(msgIn, &payload, &payloadSize_bytes) )
	{
		cxa_logger_log_untermString(&clientIn->logger, CXA_LOG_LEVEL_TRACE, "got PUBLISH '", topicName, topicNameLen_bytes, "'");

		cxa_array_iterate(&clientIn->subscriptions, currSubscription, cxa_mqtt_client_subscriptionEntry_t)
		{
			if( currSubscription == NULL ) continue;

			// @TODO this is a hack...I'm using paho's topic matching code...but it expects a null-terminated string
			// MQTT spec says all string are not null-terminated...I don't want to change the message data (add null)
			// so we'll null term for the check, then un-null term

			char oldVal = topicName[topicNameLen_bytes];
			topicName[topicNameLen_bytes] = 0;
			bool doesTopicMatch = doesTopicMatchFilter(topicName, currSubscription->topicFilter);
			topicName[topicNameLen_bytes] = oldVal;

			if( doesTopicMatch && currSubscription->cb_onPublish )
			{
				currSubscription->cb_onPublish(clientIn, msgIn, topicName, topicNameLen_bytes, payload, payloadSize_bytes, currSubscription->userVar);
			}
		}
	} else cxa_logger_warn(&clientIn->logger, "malformed PUBLISH");
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
