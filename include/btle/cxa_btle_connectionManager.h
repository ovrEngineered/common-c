/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_BTLE_CONNECTIONMANAGER_H_
#define CXA_BTLE_CONNECTIONMANAGER_H_


// ******** includes ********
#include <cxa_array.h>
#include <cxa_btle_central.h>
#include <cxa_btle_connection.h>
#include <cxa_config.h>
#include <cxa_eui48.h>
#include <cxa_logger_header.h>
#include <cxa_stateMachine.h>
#include <stdbool.h>


// ******** global macro definitions ********
#ifndef CXA_BTLE_CONNECTION_MANAGER_MAXNUM_LISTENERS
#define CXA_BTLE_CONNECTION_MANAGER_MAXNUM_LISTENERS						2
#endif

#ifndef CXA_BTLE_CONNECTION_MANAGER_MAXNUM_SUBSCRIPTION_STATE_ENTRIES
#define CXA_BTLE_CONNECTION_MANAGER_MAXNUM_SUBSCRIPTION_STATE_ENTRIES		4
#endif

#ifndef CXA_BTLE_CONNECTION_MANAGER_MAXNUM_SUBSCRIPTION_STATES
#define CXA_BTLE_CONNECTION_MANAGER_MAXNUM_SUBSCRIPTION_STATES				2
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_btle_connectionManager cxa_btle_connectionManager_t;


/**
 * @public
 */
typedef struct cxa_btle_connectionManager_subscriptionState cxa_btle_connectionManager_subscriptionState_t;


/**
 * @public
 */
typedef void (*cxa_btle_connectionManager_listenerCb_onConnected_t)(void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_connectionManager_listenerCb_onDisconnected_t)(void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_connectionManager_listenerCb_onStopped_t)(void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_connectionManager_listenerCb_onSubStateTransitionComplete_t)(cxa_btle_connectionManager_subscriptionState_t *const subStateIn, bool wasSuccessfulIn, void* userVarIn);


/**
 * @private
 */
typedef struct
{
	bool isSubscribed;

	const char* serviceUuid_str;
	const char* characteristicUuid_str;

	cxa_btle_connection_cb_onNotiIndiRx_t cb_onRx;
	void* userVar;
}cxa_btle_connectionManager_subscriptionState_entry_t;


/**
 * @private
 */
struct cxa_btle_connectionManager_subscriptionState
{
	cxa_array_t entries;
	cxa_btle_connectionManager_subscriptionState_entry_t entries_raw[CXA_BTLE_CONNECTION_MANAGER_MAXNUM_SUBSCRIPTION_STATE_ENTRIES];

	size_t currEntryIndex;
};


/**
 * @private
 */
typedef struct
{
	cxa_btle_connectionManager_listenerCb_onConnected_t cb_onConnected;
	cxa_btle_connectionManager_listenerCb_onDisconnected_t cb_onDisconnected;
	cxa_btle_connectionManager_listenerCb_onStopped_t cb_onStopped;

	cxa_btle_connectionManager_listenerCb_onSubStateTransitionComplete_t cb_onSubStateTransitionComplete;

	void *userVar;
}cxa_btle_connectionManager_listener_t;


/**
 * @private
 */
typedef enum
{
	CXA_BTLE_CONNMAN_CMD_RUN,
	CXA_BTLE_CONNMAN_CMD_STOP,
	CXA_BTLE_CONNMAN_CMD_RESTART
}cxa_btle_connectionManager_command_t;


/**
 * @private
 */
struct cxa_btle_connectionManager
{
	cxa_btle_central_t* btlec;

	cxa_eui48_t targetMacAddress;
	cxa_eui48_t nextMacAddress;
	cxa_btle_connection_t* conn;

	cxa_array_t listeners;
	cxa_btle_connectionManager_listener_t listeners_raw[CXA_BTLE_CONNECTION_MANAGER_MAXNUM_LISTENERS];

	cxa_btle_connectionManager_command_t currCommand;

	cxa_array_t subscriptionStates;
	cxa_btle_connectionManager_subscriptionState_t subscriptionStates_raw[CXA_BTLE_CONNECTION_MANAGER_MAXNUM_SUBSCRIPTION_STATES];
	cxa_btle_connectionManager_subscriptionState_t *targetSubState;

	cxa_stateMachine_t stateMachine;
	cxa_logger_t logger;
};


// ******** global function prototypes ********
void cxa_btle_connectionManager_init(cxa_btle_connectionManager_t *const btleCmIn, cxa_btle_central_t *const btlecIn, int threadIdIn);

void cxa_btle_connectionManager_addListener(cxa_btle_connectionManager_t *const btleCmIn,
											cxa_btle_connectionManager_listenerCb_onConnected_t cb_onConnectedIn,
											cxa_btle_connectionManager_listenerCb_onDisconnected_t cb_onDisconnectedIn,
											cxa_btle_connectionManager_listenerCb_onStopped_t cb_onStoppedIn,
											cxa_btle_connectionManager_listenerCb_onSubStateTransitionComplete_t cb_onSubStateTransitionCompleteIn,
											void *userVarIn);

void cxa_btle_connectionManager_start(cxa_btle_connectionManager_t *const btleCmIn, cxa_eui48_t *const targetMacIn);
void cxa_btle_connectionManager_stop(cxa_btle_connectionManager_t *const btleCmIn);
void cxa_btle_connectionManager_forceReconnect(cxa_btle_connectionManager_t *const btleCmIn);

cxa_btle_connectionManager_subscriptionState_t* cxa_btle_connectionManager_addSubscriptionState(cxa_btle_connectionManager_t *const btleCmIn);
bool cxa_btle_connectionManager_addSubscriptionStateEntry_subscribed(cxa_btle_connectionManager_subscriptionState_t *const subStateIn,
														  	  	  	 const char *const serviceUuidIn,
																	 const char *const characteristicUuidIn,
																	 cxa_btle_connection_cb_onNotiIndiRx_t cb_onRxIn,
																	 void* userVarIn);
bool cxa_btle_connectionManager_addSubscriptionStateEntry_unsubscribed(cxa_btle_connectionManager_subscriptionState_t *const subStateIn,
														  	  	  	   const char *const serviceUuidIn,
																	   const char *const characteristicUuidIn);
void cxa_btle_connectionManager_setTargetSubscriptionState(cxa_btle_connectionManager_t *const btleCmIn,
														   cxa_btle_connectionManager_subscriptionState_t *const subStateIn);

bool cxa_btle_connectionManager_isRunning(cxa_btle_connectionManager_t *const btleCmIn);
bool cxa_btle_connectionManager_isConnected(cxa_btle_connectionManager_t *const btleCmIn);
bool cxa_btle_connectionManager_getMacAddress(cxa_btle_connectionManager_t *const btleCmIn, cxa_eui48_t *const macAddrOut);

cxa_btle_connection_t* cxa_btle_connectionManager_getConnection(cxa_btle_connectionManager_t *const btleCmIn);

#endif
