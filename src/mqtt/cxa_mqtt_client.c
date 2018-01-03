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

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_INFO
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define CONNACK_TIMEOUT_MS				5000
#define SUBACK_TIMEOUT_MS				5000


// ******** local type definitions ********
typedef enum
{
	MQTT_STATE_IDLE,
	MQTT_STATE_CONNECTING_TRANSPORT,
	MQTT_STATE_CONNECTING,
	MQTT_STATE_CONNECTED,
}state_t;


// ******** local function prototypes ********
static void stateCb_idle_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_connecting_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_connected_state(cxa_stateMachine_t *const smIn, void *userVarIn);

static void protoParseCb_onIoException(void *const userVarIn);
static void protoParseCb_onPacketReceived(cxa_fixedByteBuffer_t *const packetIn, void *const userVarIn);

static void handleMessage_connAck(cxa_mqtt_client_t *const clientIn, cxa_mqtt_message_t *const msgIn);
static void handleMessage_pingResp(cxa_mqtt_client_t *const clientIn, cxa_mqtt_message_t *const msgIn);
static void handleMessage_subAck(cxa_mqtt_client_t *const clientIn, cxa_mqtt_message_t *const msgIn);
static void handleMessage_publish(cxa_mqtt_client_t *const clientIn, cxa_mqtt_message_t *const msgIn);

static bool doesTopicMatchFilter(char* topicIn, char* filterIn);
static void notify_activity(cxa_mqtt_client_t *const clientIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_mqtt_client_init(cxa_mqtt_client_t *const clientIn, cxa_ioStream_t *const iosIn, uint16_t keepAliveTimeout_sIn, char *const clientIdIn, int threadIdIn)
{
	cxa_assert(clientIn);
	cxa_assert(iosIn);
	cxa_assert(clientIdIn);

	// save our references
	clientIn->clientId = clientIdIn;
	clientIn->threadId = threadIdIn;

	// setup some initial values
	clientIn->keepAliveTimeout_s = keepAliveTimeout_sIn;
	clientIn->scm_onDisconnect = NULL;
	cxa_timeDiff_init(&clientIn->td_timeout);
	cxa_timeDiff_init(&clientIn->td_sendKeepAlive);
	cxa_timeDiff_init(&clientIn->td_receiveKeepAlive);

	// get a message (and buffer) for our protocol parser
	cxa_mqtt_message_t* msg = cxa_mqtt_messageFactory_getFreeMessage_empty();
	cxa_assert(msg);

	// setup our protocol parser
	cxa_protocolParser_mqtt_init(&clientIn->mpp, iosIn, msg->buffer, threadIdIn);
	cxa_protocolParser_addProtocolListener(&clientIn->mpp.super, protoParseCb_onIoException, NULL, (void*)clientIn);
	cxa_protocolParser_addPacketListener(&clientIn->mpp.super, protoParseCb_onPacketReceived, (void*)clientIn);

	// setup our logger
	cxa_logger_init(&clientIn->logger, "mqttC");

	// setup our listeners array
	cxa_array_initStd(&clientIn->listeners, clientIn->listeners_raw);

	// setup our subscriptions array
	cxa_array_initStd(&clientIn->subscriptions, clientIn->subscriptions_raw);

	// setup our will
	clientIn->will.topic[0] = 0;
	clientIn->will.payload[0] = 0;
	clientIn->will.payloadLen_bytes = 0;

	// setup our state machine
	cxa_stateMachine_init(&clientIn->stateMachine, "mqttClient", threadIdIn);
	cxa_stateMachine_addState(&clientIn->stateMachine, MQTT_STATE_IDLE, "idle", stateCb_idle_enter, NULL, NULL, (void*)clientIn);
	cxa_stateMachine_addState(&clientIn->stateMachine, MQTT_STATE_CONNECTING_TRANSPORT, "connectingTransport", NULL, NULL, NULL, (void*)clientIn);
	cxa_stateMachine_addState(&clientIn->stateMachine, MQTT_STATE_CONNECTING, "connecting", stateCb_connecting_enter, stateCb_connecting_state, NULL, (void*)clientIn);
	cxa_stateMachine_addState(&clientIn->stateMachine, MQTT_STATE_CONNECTED, "connected", stateCb_connected_enter, stateCb_connected_state, NULL, (void*)clientIn);
	cxa_stateMachine_transition(&clientIn->stateMachine, MQTT_STATE_IDLE);
}


bool cxa_mqtt_client_setWillMessage(cxa_mqtt_client_t *const clientIn, cxa_mqtt_qosLevel_t qosIn, bool retainIn,
									char* topicNameIn, void *const payloadIn, size_t payloadLen_bytesIn)
{
	cxa_assert(clientIn);
	if( (payloadLen_bytesIn > 0) && (payloadIn == NULL) ) return false;
	if( payloadLen_bytesIn > sizeof(clientIn->will.payload) ) return false;

	// this is how we clear a will (for future connects)
	if( topicNameIn == NULL )
	{
		clientIn->will.topic[0] = 0;
		clientIn->will.payload[0] = 0;
		clientIn->will.payloadLen_bytes = 0;
		return true;
	}

	// if we made it here, we are setting a new will
	clientIn->will.qos = qosIn;
	clientIn->will.retain = retainIn;
	cxa_assert( strlcpy(clientIn->will.topic, topicNameIn, sizeof(clientIn->will.topic)) < sizeof(clientIn->will.topic) );
	if( payloadIn != NULL ) memcpy(clientIn->will.payload, payloadIn, payloadLen_bytesIn);
	clientIn->will.payloadLen_bytes = payloadLen_bytesIn;

	return true;
}


void cxa_mqtt_client_addListener(cxa_mqtt_client_t *const clientIn,
								 cxa_mqtt_client_cb_onConnect_t cb_onConnectIn,
								 cxa_mqtt_client_cb_onConnectFailed_t cb_onConnectFailIn,
								 cxa_mqtt_client_cb_onDisconnect_t cb_onDisconnectIn,
								 cxa_mqtt_client_cb_onActivity_t cb_onActivityIn,
								 void *const userVarIn)
{
	cxa_assert(clientIn);

	cxa_mqtt_client_listenerEntry_t newEntry =
	{
		.cb_onConnect=cb_onConnectIn,
		.cb_onConnectFail=cb_onConnectFailIn,
		.cb_onDisconnect=cb_onDisconnectIn,
		.cb_onActivity=cb_onActivityIn,
		.userVar=userVarIn
	};
	cxa_assert_msg(cxa_array_append(&clientIn->listeners, &newEntry), "increase CXA_MQTT_CLIENT_MAXNUM_LISTENERS");
}


bool cxa_mqtt_client_connect(cxa_mqtt_client_t *const clientIn, char *const usernameIn, uint8_t *const passwordIn, uint16_t passwordLen_bytesIn)
{
	cxa_assert(clientIn);

	state_t currState = cxa_stateMachine_getCurrentState(&clientIn->stateMachine);
	if( (currState == MQTT_STATE_CONNECTING) ||
		(currState == MQTT_STATE_CONNECTED) ) return false;

	cxa_logger_trace(&clientIn->logger, "sending CONNECT packet");

	// reserve/initialize/send message
	cxa_mqtt_message_t* msg = NULL;
	if( ((msg = cxa_mqtt_messageFactory_getFreeMessage_empty()) == NULL) ||
			!cxa_mqtt_message_connect_init(msg, clientIn->clientId, usernameIn, passwordIn, passwordLen_bytesIn,
										   clientIn->will.qos, clientIn->will.retain, clientIn->will.topic, clientIn->will.payload, clientIn->will.payloadLen_bytes,
										   true, clientIn->keepAliveTimeout_s) ||
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

	// now let our lower-level connection know that we're disconnecting
	if( clientIn->scm_onDisconnect != NULL ) clientIn->scm_onDisconnect(clientIn);

	cxa_stateMachine_transition(&clientIn->stateMachine, MQTT_STATE_IDLE);
}


bool cxa_mqtt_client_publish(cxa_mqtt_client_t *const clientIn, cxa_mqtt_qosLevel_t qosIn, bool retainIn,
							 char* topicNameIn, void *const payloadIn, size_t payloadLen_bytesIn)
{
	cxa_assert(clientIn);
	cxa_assert(topicNameIn);

	if( !cxa_mqtt_client_isConnected(clientIn) ) return false;

	cxa_mqtt_message_t* msg = NULL;
	if( ((msg = cxa_mqtt_messageFactory_getFreeMessage_empty()) == NULL) ||
		!cxa_mqtt_message_publish_init(msg, false, qosIn, retainIn, topicNameIn, clientIn->currPacketId++, payloadIn, payloadLen_bytesIn) )
	{
		cxa_logger_warn(&clientIn->logger, "publish reserve/initialize failed, dropped");
		if( msg != NULL ) cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
		return false;
	}

	bool retVal = cxa_mqtt_client_publish_message(clientIn, msg);
	cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
	return retVal;
}


bool cxa_mqtt_client_publish_message(cxa_mqtt_client_t *const clientIn, cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(clientIn);
	cxa_assert(msgIn);

	if( !cxa_mqtt_client_isConnected(clientIn) ) return false;

	char *topicName;
	uint16_t topicNameLen_bytes;
	if( !cxa_mqtt_message_publish_getTopicName(msgIn, &topicName, &topicNameLen_bytes) ) return false;

//	cxa_logger_log_untermString(&clientIn->logger, CXA_LOG_LEVEL_INFO, "publish '", topicName, topicNameLen_bytes, "'");
	bool retVal = true;
	if( !cxa_protocolParser_writePacket(&clientIn->mpp.super, cxa_mqtt_message_getBuffer(msgIn)) )
	{
		cxa_logger_warn(&clientIn->logger, "publish send failed, dropped");
		retVal = false;
	}

	if( retVal ) notify_activity(clientIn);

	return retVal;
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


int cxa_mqtt_client_getThreadId(cxa_mqtt_client_t *const clientIn)
{
	cxa_assert(clientIn);

	return clientIn->threadId;
}


void cxa_mqtt_client_super_connectingTransport(cxa_mqtt_client_t *const clientIn)
{
	cxa_assert(clientIn);

	cxa_stateMachine_transitionNow(&clientIn->stateMachine, MQTT_STATE_CONNECTING_TRANSPORT);
}


void cxa_mqtt_client_super_connectFail(cxa_mqtt_client_t *const clientIn, cxa_mqtt_client_connectFailureReason_t reasonIn)
{
	cxa_assert(clientIn);

	clientIn->connFailReason = reasonIn;
	cxa_stateMachine_transition(&clientIn->stateMachine, MQTT_STATE_IDLE);
}


void cxa_mqtt_client_super_disconnect(cxa_mqtt_client_t *const clientIn)
{
	cxa_assert(clientIn);

	// we only want to notify our clients if we're connected
	//if( cxa_stateMachine_getCurrentState(&clientIn->stateMachine) != MQTT_STATE_CONNECTED ) return;

	// transition
	cxa_stateMachine_transition(&clientIn->stateMachine, MQTT_STATE_IDLE);
}


// ******** local function implementations ********
static void stateCb_idle_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_mqtt_client_t *clientIn = (cxa_mqtt_client_t*) userVarIn;
	cxa_assert(clientIn);

	// notify our listeners
	cxa_array_iterate(&clientIn->listeners, currListener, cxa_mqtt_client_listenerEntry_t)
	{
		if( currListener == NULL ) continue;

		switch( prevStateIdIn )
		{
			case MQTT_STATE_CONNECTING_TRANSPORT:
			case MQTT_STATE_CONNECTING:
				if( currListener->cb_onConnectFail != NULL ) currListener->cb_onConnectFail(clientIn, clientIn->connFailReason, currListener->userVar);
				break;

			case MQTT_STATE_CONNECTED:
				if( currListener->cb_onDisconnect != NULL ) currListener->cb_onDisconnect(clientIn, currListener->userVar);
				break;

			default:
				break;
		}
	}
}


static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_mqtt_client_t *clientIn = (cxa_mqtt_client_t*) userVarIn;
	cxa_assert(clientIn);

	// make sure our protocolParser isn't in an error state (from a possible previous connection)
	cxa_protocolParser_resetError(&clientIn->mpp.super);

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

		// manually transition (to maintain separation between high-level and low-level reasons)
		clientIn->connFailReason = CXA_MQTT_CLIENT_CONNECTFAIL_REASON_TIMEOUT;

		// let our lower-level connection know that we're disconnecting
		if( clientIn->scm_onDisconnect != NULL ) clientIn->scm_onDisconnect(clientIn);

		cxa_stateMachine_transition(&clientIn->stateMachine, MQTT_STATE_IDLE);
		return;
	}
}


static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_mqtt_client_t *clientIn = (cxa_mqtt_client_t*) userVarIn;
	cxa_assert(clientIn);

	cxa_logger_info(&clientIn->logger, "connected");

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
	if( (clientIn->keepAliveTimeout_s != 0) && cxa_timeDiff_isElapsed_recurring_ms(&clientIn->td_sendKeepAlive, (clientIn->keepAliveTimeout_s * 1000)) )
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
	if( (clientIn->keepAliveTimeout_s != 0) && cxa_timeDiff_isElapsed_recurring_ms(&clientIn->td_receiveKeepAlive, (clientIn->keepAliveTimeout_s * 1000 * 2)) )
	{
		cxa_logger_warn(&clientIn->logger, "no PINGRESP, server is unresponsive");

		// let our lower-level connection know that we're disconnecting
		if( clientIn->scm_onDisconnect != NULL ) clientIn->scm_onDisconnect(clientIn);
		return;
	}
}


static void protoParseCb_onIoException(void *const userVarIn)
{
	cxa_mqtt_client_t *clientIn = (cxa_mqtt_client_t*) userVarIn;
	cxa_assert(clientIn);

	cxa_logger_warn(&clientIn->logger, "ioException...disconnecting");

	// if this was during connection, make sure we record the failure reason
	if( cxa_stateMachine_getCurrentState(&clientIn->stateMachine) == MQTT_STATE_CONNECTING )
	{
		clientIn->connFailReason = CXA_MQTT_CLIENT_CONNECTFAIL_REASON_NETWORK;
	}

	// let our lower-level connection know that we're disconnecting
	if( clientIn->scm_onDisconnect != NULL ) clientIn->scm_onDisconnect(clientIn);

	cxa_stateMachine_transition(&clientIn->stateMachine, MQTT_STATE_IDLE);
	return;
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

	// only handle if we are in the appropriate state
	if( cxa_stateMachine_getCurrentState(&clientIn->stateMachine) != MQTT_STATE_CONNECTING ) return;

	cxa_mqtt_connAck_returnCode_t retCode = CXA_MQTT_CONNACK_RETCODE_UNKNOWN;
	if( cxa_mqtt_message_connack_getReturnCode(msgIn, &retCode ) && (retCode == CXA_MQTT_CONNACK_RETCODE_ACCEPTED) )
	{
		cxa_logger_trace(&clientIn->logger, "got CONNACK");

		cxa_stateMachine_transition(&clientIn->stateMachine, MQTT_STATE_CONNECTED);
		return;
	}
	else
	{
		cxa_logger_warn(&clientIn->logger, "connection refused: %d", retCode);

		// now let our lower-level connection know that we're disconnecting
		if( clientIn->scm_onDisconnect != NULL ) clientIn->scm_onDisconnect(clientIn);

		clientIn->connFailReason = CXA_MQTT_CLIENT_CONNECTFAIL_REASON_AUTH;

		// transition
		cxa_stateMachine_transition(&clientIn->stateMachine, MQTT_STATE_IDLE);
	}
}


static void handleMessage_pingResp(cxa_mqtt_client_t *const clientIn, cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(clientIn);
	cxa_assert(msgIn);

	cxa_logger_trace(&clientIn->logger, "got PINGRESP");
	cxa_timeDiff_setStartTime_now(&clientIn->td_receiveKeepAlive);

	// notify our listeners
	notify_activity(clientIn);
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
	uint16_t topicNameLen_bytes;
	cxa_linkedField_t* lf_payload;
	void *payload;
	size_t payloadSize_bytes;
	if( cxa_mqtt_message_publish_getTopicName(msgIn, &topicName, &topicNameLen_bytes) && cxa_mqtt_message_publish_getPayload(msgIn, &lf_payload) )
	{
		cxa_logger_log_untermString(&clientIn->logger, CXA_LOG_LEVEL_INFO, "got PUBLISH '", topicName, topicNameLen_bytes, "'");

		payloadSize_bytes = cxa_linkedField_getSize_bytes(lf_payload);
		payload = (payloadSize_bytes > 0) ? cxa_linkedField_get_pointerToIndex(lf_payload, 0) : NULL;

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

		// notify our listeners
		notify_activity(clientIn);
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


static void notify_activity(cxa_mqtt_client_t *const clientIn)
{
	cxa_assert(clientIn);

	cxa_array_iterate(&clientIn->listeners, currListener, cxa_mqtt_client_listenerEntry_t)
	{
		if( currListener == NULL ) continue;
		if( currListener->cb_onActivity != NULL ) currListener->cb_onActivity(clientIn, currListener->userVar);
	}
}
