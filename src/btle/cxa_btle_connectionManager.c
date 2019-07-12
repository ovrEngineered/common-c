/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_btle_connectionManager.h"


// ******** includes ********
#include <cxa_assert.h>


#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_DEBUG
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define CONNECT_STANDOFF_MS									5000


// ******** local type definitions ********
typedef enum
{
	STATE_STOPPED,
	STATE_WAIT_FOR_BTLEC_READY,
	STATE_CONNECTING,
	STATE_CONNECTED,
	STATE_DISCONNECTING,
	STATE_CONNECT_STANDOFF,
}state_t;


// ******** local function prototypes ********
static void notifyListeners_onConnected(cxa_btle_connectionManager_t *const btleCmIn);
static void notifyListeners_onDisconnected(cxa_btle_connectionManager_t *const btleCmIn);
static void notifyListeners_onSubStateTransitionComplete(cxa_btle_connectionManager_t *const btleCmIn, bool wasSuccessfulIn);

static void executeNextSubscriptionEntry(cxa_btle_connectionManager_t *const btleCmIn);

static void btleCb_onConnectionOpened(bool wasSuccessfulIn, cxa_btle_connection_t *const connectionIn, void* userVarIn);
static void btleCb_onConnectionClosed(cxa_btle_connection_disconnectReason_t reasonIn, void* userVarIn);

static void btleCb_onNotiIndiSubscriptionChanged(const char *const serviceUuidIn, const char *const characteristicUuidIn, bool wasSuccessfulIn, void* userVarIn);
static void btleCb_onNotiIndiRx(const char *const serviceUuidIn, const char *const characteristicUuidIn, cxa_fixedByteBuffer_t *fbb_readDataIn, void* userVarIn);

static void stateCb_waitForBtlecReady_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_waitForBtlecReady_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_connected_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_disconnecting_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_btle_connectionManager_init(cxa_btle_connectionManager_t *const btleCmIn, cxa_btle_central_t *const btlecIn, int threadIdIn)
{
	cxa_assert(btleCmIn);

	// setup our internal state
	btleCmIn->btlec = btlecIn;
	cxa_logger_init(&btleCmIn->logger, "btlecm");
	cxa_array_initStd(&btleCmIn->listeners, btleCmIn->listeners_raw);
	cxa_array_initStd(&btleCmIn->subscriptionStates, btleCmIn->subscriptionStates_raw);
	btleCmIn->currCommand = CXA_BTLE_CONNMAN_CMD_STOP;

	// setup our stateMachine
	cxa_stateMachine_init(&btleCmIn->stateMachine, "btlecm", threadIdIn);
	cxa_stateMachine_addState(&btleCmIn->stateMachine, STATE_STOPPED, "stopped", NULL, NULL, NULL, (void*)btleCmIn);
	cxa_stateMachine_addState(&btleCmIn->stateMachine, STATE_WAIT_FOR_BTLEC_READY, "waitForBtleC", stateCb_waitForBtlecReady_enter, stateCb_waitForBtlecReady_state, NULL, (void*)btleCmIn);
	cxa_stateMachine_addState(&btleCmIn->stateMachine, STATE_CONNECTING, "connecting", stateCb_connecting_enter, NULL, NULL, (void*)btleCmIn);
	cxa_stateMachine_addState(&btleCmIn->stateMachine, STATE_CONNECTED, "connected", stateCb_connected_enter, NULL, stateCb_connected_leave, (void*)btleCmIn);
	cxa_stateMachine_addState(&btleCmIn->stateMachine, STATE_DISCONNECTING, "disconnecting", stateCb_disconnecting_enter, NULL, NULL, (void*)btleCmIn);
	cxa_stateMachine_addState_timed(&btleCmIn->stateMachine, STATE_CONNECT_STANDOFF, "connStandoff", STATE_CONNECTING, CONNECT_STANDOFF_MS, NULL, NULL, NULL, (void*)btleCmIn);
	cxa_stateMachine_setInitialState(&btleCmIn->stateMachine, STATE_STOPPED);
}


void cxa_btle_connectionManager_addListener(cxa_btle_connectionManager_t *const btleCmIn,
											cxa_btle_connectionManager_listenerCb_onConnected_t cb_onConnectedIn,
											cxa_btle_connectionManager_listenerCb_onDisconnected_t cb_onDisconnectedIn,
											cxa_btle_connectionManager_listenerCb_onStopped_t cb_onStoppedIn,
											cxa_btle_connectionManager_listenerCb_onSubStateTransitionComplete_t cb_onSubStateTransitionCompleteIn,
											void *userVarIn)
{
	cxa_assert(btleCmIn);

	cxa_btle_connectionManager_listener_t newListener = {
			.cb_onConnected = cb_onConnectedIn,
			.cb_onDisconnected = cb_onDisconnectedIn,
			.cb_onStopped = cb_onStoppedIn,
			.cb_onSubStateTransitionComplete = cb_onSubStateTransitionCompleteIn,
			.userVar = userVarIn
	};
	cxa_assert(cxa_array_append(&btleCmIn->listeners, &newListener));
}


void cxa_btle_connectionManager_start(cxa_btle_connectionManager_t *const btleCmIn, cxa_eui48_t *const targetMacIn)
{
	cxa_assert(btleCmIn);

	// depends on our current state
	switch( cxa_stateMachine_getCurrentState(&btleCmIn->stateMachine) )
	{
		case STATE_STOPPED:
		case STATE_WAIT_FOR_BTLEC_READY:
		case STATE_CONNECT_STANDOFF:
		case STATE_DISCONNECTING:
			cxa_eui48_initFromEui48(&btleCmIn->targetMacAddress, targetMacIn);
			btleCmIn->currCommand = CXA_BTLE_CONNMAN_CMD_RUN;
			cxa_stateMachine_transition(&btleCmIn->stateMachine, STATE_WAIT_FOR_BTLEC_READY);
			break;

		case STATE_CONNECTING:
			// we're already trying to connect to something else...flag it for later
			cxa_eui48_initFromEui48(&btleCmIn->nextMacAddress, targetMacIn);
			btleCmIn->currCommand = CXA_BTLE_CONNMAN_CMD_RESTART;
			break;

		case STATE_CONNECTED:
			// we're already connected to something else so don't modify our current mac...flag it for later
			cxa_eui48_initFromEui48(&btleCmIn->nextMacAddress, targetMacIn);
			btleCmIn->currCommand = CXA_BTLE_CONNMAN_CMD_RESTART;
			cxa_stateMachine_transition(&btleCmIn->stateMachine, STATE_DISCONNECTING);
			break;
	}
}


void cxa_btle_connectionManager_stop(cxa_btle_connectionManager_t *const btleCmIn)
{
	cxa_assert(btleCmIn);

	// set our flag for all cases
	btleCmIn->currCommand = CXA_BTLE_CONNMAN_CMD_STOP;

	switch( cxa_stateMachine_getCurrentState(&btleCmIn->stateMachine) )
	{
		case STATE_STOPPED:
			break;

		case STATE_WAIT_FOR_BTLEC_READY:
		case STATE_CONNECT_STANDOFF:
			cxa_stateMachine_transition(&btleCmIn->stateMachine, STATE_STOPPED);
			break;

		case STATE_CONNECTING:
		case STATE_DISCONNECTING:
			// let the internal state checks take care of this
			break;

		case STATE_CONNECTED:
			cxa_stateMachine_transition(&btleCmIn->stateMachine, STATE_DISCONNECTING);
			break;
	}
}


void cxa_btle_connectionManager_forceReconnect(cxa_btle_connectionManager_t *const btleCmIn)
{
	cxa_assert(btleCmIn);

	// depends on our current state
	switch( cxa_stateMachine_getCurrentState(&btleCmIn->stateMachine) )
	{
		case STATE_STOPPED:
		case STATE_WAIT_FOR_BTLEC_READY:
		case STATE_CONNECT_STANDOFF:
		case STATE_DISCONNECTING:
			// do nothing
			break;

		case STATE_CONNECTING:
			// we're already trying to connect...flag it for later
			cxa_eui48_initFromEui48(&btleCmIn->nextMacAddress, &btleCmIn->targetMacAddress);
			btleCmIn->currCommand = CXA_BTLE_CONNMAN_CMD_RESTART;
			break;

		case STATE_CONNECTED:
			// we're already connected...flag it for later
			cxa_eui48_initFromEui48(&btleCmIn->nextMacAddress, &btleCmIn->targetMacAddress);
			btleCmIn->currCommand = CXA_BTLE_CONNMAN_CMD_RESTART;
			cxa_stateMachine_transition(&btleCmIn->stateMachine, STATE_DISCONNECTING);
			break;
	}
}


cxa_btle_connectionManager_subscriptionState_t* cxa_btle_connectionManager_addSubscriptionState(cxa_btle_connectionManager_t *const btleCmIn)
{
	cxa_assert(btleCmIn);

	cxa_btle_connectionManager_subscriptionState_t* retVal = cxa_array_append_empty(&btleCmIn->subscriptionStates);
	if( retVal != NULL )
	{
		cxa_array_initStd(&retVal->entries, retVal->entries_raw);
	}

	return retVal;
}


bool cxa_btle_connectionManager_addSubscriptionStateEntry_subscribed(cxa_btle_connectionManager_subscriptionState_t *const subStateIn,
														  const char *const serviceUuidIn,
														  const char *const characteristicUuidIn,
														  cxa_btle_connection_cb_onNotiIndiRx_t cb_onRxIn,
														  void* userVarIn)
{
	cxa_assert(subStateIn);

	cxa_btle_connectionManager_subscriptionState_entry_t newEntry;
	newEntry.isSubscribed = true;
	if( !cxa_btle_uuid_initFromString(&newEntry.uuid_service, serviceUuidIn) ) return false;
	if( !cxa_btle_uuid_initFromString(&newEntry.uuid_characteristic, characteristicUuidIn) ) return false;
	newEntry.cb_onRx = cb_onRxIn;
	newEntry.userVar = userVarIn;

	return cxa_array_append(&subStateIn->entries, &newEntry);
}


bool cxa_btle_connectionManager_addSubscriptionStateEntry_unsubscribed(cxa_btle_connectionManager_subscriptionState_t *const subStateIn,
														  const char *const serviceUuidIn,
														  const char *const characteristicUuidIn)
{
	cxa_assert(subStateIn);

	cxa_btle_connectionManager_subscriptionState_entry_t newEntry;
	newEntry.isSubscribed = false;
	if( !cxa_btle_uuid_initFromString(&newEntry.uuid_service, serviceUuidIn) ) return false;
	if( !cxa_btle_uuid_initFromString(&newEntry.uuid_characteristic, characteristicUuidIn) ) return false;
	newEntry.cb_onRx = NULL;
	newEntry.userVar = NULL;

	return cxa_array_append(&subStateIn->entries, &newEntry);
}


void cxa_btle_connectionManager_setTargetSubscriptionState(cxa_btle_connectionManager_t *const btleCmIn,
														   cxa_btle_connectionManager_subscriptionState_t *const subStateIn)
{
	cxa_assert(btleCmIn);
	cxa_assert(subStateIn);

	// make sure we're not already in this state
	if( subStateIn == btleCmIn->targetSubState )
	{
		cxa_logger_debug(&btleCmIn->logger, "already in this sub state");
		notifyListeners_onSubStateTransitionComplete(btleCmIn, true);
		return;
	}

	// save our target state
	btleCmIn->targetSubState = subStateIn;
	btleCmIn->targetSubState->currEntryIndex = 0;

	// start the execution to the target state (if we're connected)
	// if we're not connected, the execution will happen automatically when reconnected
	if( cxa_btle_connectionManager_isConnected(btleCmIn) )
	{
		executeNextSubscriptionEntry(btleCmIn);
	}
}


bool cxa_btle_connectionManager_isRunning(cxa_btle_connectionManager_t *const btleCmIn)
{
	cxa_assert(btleCmIn);

	return cxa_stateMachine_getCurrentState(&btleCmIn->stateMachine) != STATE_STOPPED;
}


bool cxa_btle_connectionManager_isConnected(cxa_btle_connectionManager_t *const btleCmIn)
{
	cxa_assert(btleCmIn);

	return cxa_stateMachine_getCurrentState(&btleCmIn->stateMachine) == STATE_CONNECTED;
}


bool cxa_btle_connectionManager_getMacAddress(cxa_btle_connectionManager_t *const btleCmIn, cxa_eui48_t *const macAddrOut)
{
	cxa_assert(btleCmIn);

	if( !cxa_btle_connectionManager_isRunning(btleCmIn) ) return false;


	if( macAddrOut != NULL ) cxa_eui48_initFromEui48(macAddrOut, &btleCmIn->targetMacAddress);

	return true;
}


cxa_btle_connection_t* cxa_btle_connectionManager_getConnection(cxa_btle_connectionManager_t *const btleCmIn)
{
	cxa_assert(btleCmIn);

	return cxa_btle_connectionManager_isConnected(btleCmIn) ? btleCmIn->conn : NULL;
}


// ******** local function implementations ********
static void notifyListeners_onConnected(cxa_btle_connectionManager_t *const btleCmIn)
{
	cxa_assert(btleCmIn);

	cxa_array_iterate(&btleCmIn->listeners, currListener, cxa_btle_connectionManager_listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_onConnected != NULL ) currListener->cb_onConnected(currListener->userVar);
	}
}


static void notifyListeners_onDisconnected(cxa_btle_connectionManager_t *const btleCmIn)
{
	cxa_assert(btleCmIn);

	cxa_array_iterate(&btleCmIn->listeners, currListener, cxa_btle_connectionManager_listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_onDisconnected != NULL ) currListener->cb_onDisconnected(currListener->userVar);
	}
}


static void notifyListeners_onSubStateTransitionComplete(cxa_btle_connectionManager_t *const btleCmIn, bool wasSuccessfulIn)
{
	cxa_assert(btleCmIn);

	cxa_array_iterate(&btleCmIn->listeners, currListener, cxa_btle_connectionManager_listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_onSubStateTransitionComplete != NULL ) currListener->cb_onSubStateTransitionComplete(btleCmIn->targetSubState, wasSuccessfulIn, currListener->userVar);
	}
}


static void executeNextSubscriptionEntry(cxa_btle_connectionManager_t *const btleCmIn)
{
	cxa_assert(btleCmIn);

	// don't do anything if we don't have a target state
	if( btleCmIn->targetSubState == NULL ) return;

	cxa_btle_connectionManager_subscriptionState_entry_t* currEntry = cxa_array_get(&btleCmIn->targetSubState->entries, btleCmIn->targetSubState->currEntryIndex);
	if( currEntry == NULL )
	{
		// we're done with this transition
		cxa_logger_debug(&btleCmIn->logger, "subscription state transition complete");
		notifyListeners_onSubStateTransitionComplete(btleCmIn, true);
		return;
	}
	// if we made it here, we have another subscription entry to process

	cxa_logger_debug(&btleCmIn->logger, "executing subscription entry %d / %d", btleCmIn->targetSubState->currEntryIndex+1, cxa_array_getSize_elems(&btleCmIn->targetSubState->entries));
	cxa_btle_uuid_string_t serviceUuidStr, charUuidStr;
	cxa_btle_uuid_toString(&currEntry->uuid_service, &serviceUuidStr);
	cxa_btle_uuid_toString(&currEntry->uuid_characteristic, &charUuidStr);

	if( currEntry->isSubscribed )
	{
		cxa_btle_connection_subscribeToNotifications(btleCmIn->conn,
													 serviceUuidStr.str,
													 charUuidStr.str,
													 btleCb_onNotiIndiSubscriptionChanged,
													 btleCb_onNotiIndiRx,
													 (void*)btleCmIn);
	}
	else
	{
		cxa_btle_connection_unsubscribeToNotifications(btleCmIn->conn,
													   serviceUuidStr.str,
													   charUuidStr.str,
													   btleCb_onNotiIndiSubscriptionChanged,
													   (void*)btleCmIn);
	}
}



static void btleCb_onConnectionOpened(bool wasSuccessfulIn, cxa_btle_connection_t *const connectionIn, void* userVarIn)
{
	cxa_btle_connectionManager_t *const btleCmIn = (cxa_btle_connectionManager_t *const)userVarIn;
	cxa_assert(btleCmIn);

	// make sure we were successful
	if( !wasSuccessfulIn )
	{
		cxa_logger_debug(&btleCmIn->logger, "connection failed, will retry");

		// we're in a good state to check our command before going any further
		switch( btleCmIn->currCommand )
		{
			case CXA_BTLE_CONNMAN_CMD_RUN:
			case CXA_BTLE_CONNMAN_CMD_RESTART:
				cxa_stateMachine_transition(&btleCmIn->stateMachine, STATE_CONNECT_STANDOFF);
				break;

			case CXA_BTLE_CONNMAN_CMD_STOP:
				// we're good!
				cxa_stateMachine_transition(&btleCmIn->stateMachine, STATE_STOPPED);
				break;
		}
		return;
	}

	// if we made it here we were successful...
	cxa_logger_info(&btleCmIn->logger, "connected");

	btleCmIn->conn = connectionIn;
	cxa_btle_connection_setOnClosedCb(btleCmIn->conn, btleCb_onConnectionClosed, (void*)btleCmIn);
	cxa_stateMachine_transition(&btleCmIn->stateMachine, STATE_CONNECTED);
}


static void btleCb_onConnectionClosed(cxa_btle_connection_disconnectReason_t reasonIn, void* userVarIn)
{
	cxa_btle_connectionManager_t *const btleCmIn = (cxa_btle_connectionManager_t *const)userVarIn;
	cxa_assert(btleCmIn);

	cxa_logger_debug(&btleCmIn->logger, "connection closed reason %d, %s",
					 reasonIn,
					 ((btleCmIn->currCommand == CXA_BTLE_CONNMAN_CMD_STOP) ? "stopping" : "retrying"));

	// forget our current connection
	btleCmIn->conn = NULL;

	// we're in a good state to check our command before going any further
	switch( btleCmIn->currCommand )
	{
		case CXA_BTLE_CONNMAN_CMD_RUN:
		case CXA_BTLE_CONNMAN_CMD_RESTART:
			cxa_stateMachine_transition(&btleCmIn->stateMachine, STATE_CONNECT_STANDOFF);
			break;

		case CXA_BTLE_CONNMAN_CMD_STOP:
			// we're good!
			cxa_stateMachine_transition(&btleCmIn->stateMachine, STATE_STOPPED);
			return;
	}
}


static void btleCb_onNotiIndiSubscriptionChanged(const char *const serviceUuidIn, const char *const characteristicUuidIn, bool wasSuccessfulIn, void* userVarIn)
{
	cxa_btle_connectionManager_t *const btleCmIn = (cxa_btle_connectionManager_t *const)userVarIn;
	cxa_assert(btleCmIn);

	// make sure we have a target subscription state
	if( btleCmIn->targetSubState == NULL ) return;

	// now make sure we were successful
	if( !wasSuccessfulIn )
	{
		cxa_logger_warn(&btleCmIn->logger, "failed to change subscription to %s::%s", serviceUuidIn, characteristicUuidIn);
		notifyListeners_onSubStateTransitionComplete(btleCmIn, false);
		return;
	}

	// if we made it here, we were successful...onto the next entry
	btleCmIn->targetSubState->currEntryIndex++;
	executeNextSubscriptionEntry(btleCmIn);
}


static void btleCb_onNotiIndiRx(const char *const serviceUuidIn, const char *const characteristicUuidIn, cxa_fixedByteBuffer_t *fbb_readDataIn, void* userVarIn)
{
	cxa_btle_connectionManager_t *const btleCmIn = (cxa_btle_connectionManager_t *const)userVarIn;
	cxa_assert(btleCmIn);

	// make sure we have a target subscription state
	if( btleCmIn->targetSubState == NULL ) return;

	// now find our subscription entry
	cxa_array_iterate(&btleCmIn->targetSubState->entries, currEntry, cxa_btle_connectionManager_subscriptionState_entry_t)
	{
		if( currEntry == NULL ) continue;

		if( cxa_btle_uuid_isEqualToString(&currEntry->uuid_service, serviceUuidIn) &&
			cxa_btle_uuid_isEqualToString(&currEntry->uuid_characteristic, characteristicUuidIn) &&
			currEntry->isSubscribed &&
			(currEntry->cb_onRx != NULL) )
		{
			currEntry->cb_onRx(serviceUuidIn, characteristicUuidIn, fbb_readDataIn, currEntry->userVar);
		}
	}
}


static void stateCb_waitForBtlecReady_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_btle_connectionManager_t *const btleCmIn = (cxa_btle_connectionManager_t *const)userVarIn;
	cxa_assert(btleCmIn);

	cxa_logger_debug(&btleCmIn->logger, "waiting for btlec ready");
}


static void stateCb_waitForBtlecReady_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_btle_connectionManager_t *const btleCmIn = (cxa_btle_connectionManager_t *const)userVarIn;
	cxa_assert(btleCmIn);

	// wait for our btle client to be ready
	if( cxa_btle_central_getState(btleCmIn->btlec) == CXA_BTLE_CENTRAL_STATE_READY )
	{
		cxa_stateMachine_transition(&btleCmIn->stateMachine, STATE_CONNECTING);
	}
}


static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_btle_connectionManager_t *const btleCmIn = (cxa_btle_connectionManager_t *const)userVarIn;
	cxa_assert(btleCmIn);

	// we're in a good state to check our command before going any further
	switch( btleCmIn->currCommand )
	{
		case CXA_BTLE_CONNMAN_CMD_RUN:
			// nothing special to do here
			break;

		case CXA_BTLE_CONNMAN_CMD_RESTART:
			// copy over our new mac then run normally
			cxa_eui48_initFromEui48(&btleCmIn->targetMacAddress, &btleCmIn->nextMacAddress);
			btleCmIn->currCommand = CXA_BTLE_CONNMAN_CMD_RUN;
			break;

		case CXA_BTLE_CONNMAN_CMD_STOP:
			// we're good!
			cxa_stateMachine_transition(&btleCmIn->stateMachine, STATE_STOPPED);
			return;
	}

	cxa_logger_debug(&btleCmIn->logger, "connecting");

	// we're good to start our connection
	cxa_btle_central_startConnection(btleCmIn->btlec, &btleCmIn->targetMacAddress, false,
									 btleCb_onConnectionOpened, (void*)btleCmIn);
}


static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_btle_connectionManager_t *const btleCmIn = (cxa_btle_connectionManager_t *const)userVarIn;
	cxa_assert(btleCmIn);

	// we're in a good state to check our command before going any further
	switch( btleCmIn->currCommand )
	{
		case CXA_BTLE_CONNMAN_CMD_RUN:
			// nothing special to do here
			break;

		case CXA_BTLE_CONNMAN_CMD_RESTART:
			cxa_stateMachine_transition(&btleCmIn->stateMachine, STATE_DISCONNECTING);
			return;

		case CXA_BTLE_CONNMAN_CMD_STOP:
			cxa_stateMachine_transition(&btleCmIn->stateMachine, STATE_STOPPED);
			return;
	}

	// if we made it here, we're supposed to be connected...notify our listeners
	notifyListeners_onConnected(btleCmIn);

	// see if we need to restore a subscription state
	if( btleCmIn->targetSubState != NULL )
	{
		btleCmIn->targetSubState->currEntryIndex = 0;
		executeNextSubscriptionEntry(btleCmIn);
	}
}


static void stateCb_connected_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	cxa_btle_connectionManager_t *const btleCmIn = (cxa_btle_connectionManager_t *const)userVarIn;
	cxa_assert(btleCmIn);

	// notify our listeners
	notifyListeners_onDisconnected(btleCmIn);
}


static void stateCb_disconnecting_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_btle_connectionManager_t *const btleCmIn = (cxa_btle_connectionManager_t *const)userVarIn;
	cxa_assert(btleCmIn);

	if( btleCmIn->conn != NULL )
	{
		cxa_btle_connection_stop(btleCmIn->conn);
	}
}
