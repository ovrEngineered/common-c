/**
 * @file
 * @copyright 2016 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_BTLE_CLIENT_H_
#define CXA_BTLE_CLIENT_H_


// ******** includes ********
#include <stdbool.h>
#include <stdint.h>

#include <cxa_array.h>
#include <cxa_btle_advPacket.h>
#include <cxa_btle_uuid.h>
#include <cxa_eui48.h>
#include <cxa_fixedByteBuffer.h>
#include <cxa_uuid128.h>


// ******** global macro definitions ********
#ifndef CXA_BTLE_CLIENT_MAXNUM_LISTENERS
	#define CXA_BTLE_CLIENT_MAXNUM_LISTENERS				2
#endif

#ifndef CXA_BTLE_CLIENT_MAXNUM_NOTIINDI_SUBSCRIPTIONS
	#define CXA_BTLE_CLIENT_MAXNUM_NOTIINDI_SUBSCRIPTIONS	2
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_btle_client cxa_btle_client_t;


/**
 * @public
 */
typedef enum
{
	CXA_BTLE_CLIENT_STATE_STARTUP,
	CXA_BTLE_CLIENT_STATE_STARTUPFAILED,
	CXA_BTLE_CLIENT_STATE_READY,
}cxa_btle_client_state_t;


/**
 * @public
 */
typedef enum
{
	CXA_BTLE_CLIENT_DISCONNECT_REASON_USER_REQUESTED,
	CXA_BTLE_CLIENT_DISCONNECT_REASON_CONNECTION_TIMEOUT,
	CXA_BTLE_CLIENT_DISCONNECT_REASON_STACK
}cxa_btle_client_disconnectReason_t;


/**
 * @public
 */
typedef void (*cxa_btle_client_cb_onReady_t)(cxa_btle_client_t *const btlecIn, void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_client_cb_onFailedInit_t)(cxa_btle_client_t *const btlecIn, bool willAutoRetryIn, void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_client_cb_onAdvertRx_t)(cxa_btle_advPacket_t* packetIn, void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_client_cb_onScanResponseRx_t)(void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_client_cb_onScanStart_t)(bool wasSuccessfulIn, void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_client_cb_onScanStop_t)(void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_client_cb_onConnectionOpened_t)(cxa_eui48_t *const targetAddrIn, void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_client_cb_onConnectionClosed_expected_t)(cxa_eui48_t *const targetAddrIn, void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_client_cb_onConnectionClosed_unexpected_t)(cxa_eui48_t *const targetAddrIn, cxa_btle_client_disconnectReason_t reasonIn, void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_client_cb_onReadComplete_t)(cxa_eui48_t *const targetAddrIn,
													const char *const serviceUuidIn,
													const char *const characteristicUuidIn,
													bool wasSuccessfulIn,
													cxa_fixedByteBuffer_t *fbb_readDataIn,
													void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_client_cb_onWriteComplete_t)(cxa_eui48_t *const targetAddrIn,
													 const char *const serviceUuidIn,
													 const char *const characteristicUuidIn,
													 bool wasSuccessfulIn, void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_client_cb_onNotiIndiSubscriptionChanged_t)(cxa_eui48_t *const targetAddrIn,
														  	  	   const char *const serviceUuidIn,
																   const char *const characteristicUuidIn,
																   bool wasSuccessfulIn,
																   void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_client_cb_onNotiIndiRx_t)(cxa_eui48_t *const targetAddrIn,
												  const char *const serviceUuidIn,
												  const char *const characteristicUuidIn,
												  cxa_fixedByteBuffer_t *fbb_readDataIn,
												  void* userVarIn);


/**
 * @private
 */
typedef cxa_btle_client_state_t (*cxa_btle_client_scm_getState_t)(cxa_btle_client_t *const superIn);


/**
 * @private
 */
typedef void (*cxa_btle_client_scm_startScan_t)(cxa_btle_client_t *const superIn, bool isActiveIn);


/**
 * @private
 */
typedef void (*cxa_btle_client_scm_stopScan_t)(cxa_btle_client_t *const superIn);


/**
 * @private
 */
typedef void (*cxa_btle_client_scm_startConnection_t)(cxa_btle_client_t *const superIn, cxa_eui48_t *const addrIn, bool isRandomAddrIn);


/**
 * @private
 */
typedef void (*cxa_btle_client_scm_stopConnection_t)(cxa_btle_client_t *const superIn, cxa_eui48_t *const targetAddrIn);


/**
 * @private
 */
typedef void (*cxa_btle_client_scm_readFromCharacteristic_t)(cxa_btle_client_t *const superIn,
															 cxa_eui48_t *const targetAddrIn,
															 const char *const serviceUuidIn,
															 const char *const characteristicUuidIn);


/**
 * @private
 */
typedef void (*cxa_btle_client_scm_writeToCharacteristic_t)(cxa_btle_client_t *const superIn,
															cxa_eui48_t *const targetAddrIn,
															const char *const serviceUuidIn,
															const char *const characteristicUuidIn,
															cxa_fixedByteBuffer_t *const dataIn);

/**
 * @private
 */
typedef void (*cxa_btle_client_scm_changeNotifications_t)(cxa_btle_client_t *const superIn,
														  cxa_eui48_t *const targetAddrIn,
														  const char *const serviceUuidIn,
														  const char *const characteristicUuidIn,
														  bool enableNotifications);


/**
 * @private
 */
typedef struct
{
	cxa_btle_client_cb_onReady_t cb_onReady;
	cxa_btle_client_cb_onFailedInit_t cb_onFailedInit;
	void* userVar;
}cxa_btle_client_listener_entry_t;


/**
 * @private
 */
typedef struct
{
	cxa_eui48_t address;
	cxa_btle_uuid_t uuid_service;
	cxa_btle_uuid_t uuid_characteristic;

	cxa_btle_client_cb_onNotiIndiSubscriptionChanged_t cb_onSubscriptionChanged;
	cxa_btle_client_cb_onNotiIndiRx_t cb_onRx;
	void* userVar;
}cxa_btle_client_notiIndiSubscription_t;


/**
 * @private
 */
struct cxa_btle_client
{
	cxa_array_t listeners;
	cxa_btle_client_listener_entry_t listeners_raw[CXA_BTLE_CLIENT_MAXNUM_LISTENERS];

	cxa_array_t notiIndiSubs;
	cxa_btle_client_notiIndiSubscription_t notiIndiSubs_raw[CXA_BTLE_CLIENT_MAXNUM_NOTIINDI_SUBSCRIPTIONS];

	bool hasActivityAvailable;

	struct
	{
		struct
		{
			cxa_btle_client_cb_onAdvertRx_t onAdvert;
			cxa_btle_client_cb_onScanResponseRx_t onScanResp;
			cxa_btle_client_cb_onScanStart_t onScanStart;
			cxa_btle_client_cb_onScanStop_t onScanStop;
			void *userVar;
		}scanning;

		struct
		{
			cxa_btle_client_cb_onConnectionOpened_t onConnectionOpened;
			cxa_btle_client_cb_onConnectionClosed_unexpected_t onConnectionClosed_unexpected;
			cxa_btle_client_cb_onConnectionClosed_expected_t onConnectionClosed_expected;
			void* userVar;
		}connecting;

		struct
		{
			cxa_btle_client_cb_onReadComplete_t onReadComplete;
			void* userVar;
		}reading;

		struct
		{
			cxa_btle_client_cb_onWriteComplete_t onWriteComplete;
			void* userVar;
		}writing;

		struct
		{
			cxa_btle_client_cb_onNotiIndiSubscriptionChanged_t onUnsubscribed;
			void* userVar;
		}unsubscribing;
	}cbs;

	struct
	{
		cxa_btle_client_scm_getState_t getState;

		cxa_btle_client_scm_startScan_t startScan;
		cxa_btle_client_scm_stopScan_t stopScan;

		cxa_btle_client_scm_startConnection_t startConnection;
		cxa_btle_client_scm_stopConnection_t stopConnection;

		cxa_btle_client_scm_readFromCharacteristic_t readFromCharacteristic;
		cxa_btle_client_scm_writeToCharacteristic_t writeToCharacteristic;

		cxa_btle_client_scm_changeNotifications_t changeNotifications;
	}scms;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_btle_client_init(cxa_btle_client_t *const btlecIn,
						  cxa_btle_client_scm_getState_t scm_getStateIn,
						  cxa_btle_client_scm_startScan_t scm_startScanIn,
						  cxa_btle_client_scm_stopScan_t scm_stopScanIn,
						  cxa_btle_client_scm_startConnection_t scm_startConnectionIn,
						  cxa_btle_client_scm_stopConnection_t scm_stopConnectionIn,
						  cxa_btle_client_scm_readFromCharacteristic_t scm_readFromCharIn,
						  cxa_btle_client_scm_writeToCharacteristic_t scm_writeToCharIn,
						  cxa_btle_client_scm_changeNotifications_t scm_changeNotificationsIn);


/**
 * @public
 */
void cxa_btle_client_addListener(cxa_btle_client_t *const btlecIn,
								 cxa_btle_client_cb_onReady_t cb_onReadyIn,
								 cxa_btle_client_cb_onFailedInit_t cb_onFailedInitIn,
								 void* userVarIn);


/**
 * @public
 */
void cxa_btle_client_startScan_passive(cxa_btle_client_t *const btlecIn,
									   cxa_btle_client_cb_onScanStart_t cb_scanStartIn,
									   cxa_btle_client_cb_onAdvertRx_t cb_advIn,
									   void* userVarIn);


/**
 * @public
 */
void cxa_btle_client_startScan_active(cxa_btle_client_t *const btlecIn,
									  cxa_btle_client_cb_onScanStart_t cb_scanStartIn,
									  cxa_btle_client_cb_onAdvertRx_t cb_advIn,
									  cxa_btle_client_cb_onScanResponseRx_t cb_scanRespIn,
									  void* userVarIn);


/**
 * @public
 */
void cxa_btle_client_stopScan(cxa_btle_client_t *const btlecIn,
							  cxa_btle_client_cb_onScanStop_t cb_scanStopIn,
							  void* userVarIn);


/**
 * @public
 */
cxa_btle_client_state_t cxa_btle_client_getState(cxa_btle_client_t *const btlecIn);


/**
 * @public
 */
bool cxa_btle_client_hasActivityAvailable(cxa_btle_client_t *const btlecIn);


/**
 * @public
 */
void cxa_btle_client_startConnection(cxa_btle_client_t *const btlecIn, cxa_eui48_t *const addrIn, bool isRandomAddrIn,
									 cxa_btle_client_cb_onConnectionOpened_t cb_connectionOpenedIn,
									 cxa_btle_client_cb_onConnectionClosed_unexpected_t cb_connectionClosed_unexpectedIn,
									 void* userVarIn);


/**
 * @public
 */
void cxa_btle_client_stopConnection(cxa_btle_client_t *const btlecIn,
									cxa_eui48_t *const targetAddrIn,
									cxa_btle_client_cb_onConnectionClosed_expected_t cb_connectionClosed_expectedIn,
									void *userVarIn);


/**
 * @public
 */
void cxa_btle_client_readFromCharacteristic(cxa_btle_client_t *const btlecIn,
										    cxa_eui48_t *const targetAddrIn,
										    const char *const serviceUuidIn,
										    const char *const characteristicUuidIn,
										    cxa_btle_client_cb_onReadComplete_t cb_readCompleteIn,
										    void* userVarIn);


/**
 * @public
 */
void cxa_btle_client_writeToCharacteristic_fbb(cxa_btle_client_t *const btlecIn,
											   cxa_eui48_t *const targetAddrIn,
										   	   const char *const serviceUuidIn,
											   const char *const characteristicUuidIn,
											   cxa_fixedByteBuffer_t *const dataIn,
											   cxa_btle_client_cb_onWriteComplete_t cb_writeCompleteIn,
											   void* userVarIn);


/**
 * @public
 */
void cxa_btle_client_writeToCharacteristic(cxa_btle_client_t *const btlecIn,
										   cxa_eui48_t *const targetAddrIn,
										   const char *const serviceUuidIn,
										   const char *const characteristicUuidIn,
										   void *const dataIn,
										   size_t numBytesIn,
										   cxa_btle_client_cb_onWriteComplete_t cb_writeCompleteIn,
										   void* userVarIn);


/**
 * @public
 */
void cxa_btle_client_subscribeToNotifications(cxa_btle_client_t *const btlecIn,
	    									  cxa_eui48_t *const targetAddrIn,
											  const char *const serviceUuidIn,
											  const char *const characteristicUuidIn,
											  cxa_btle_client_cb_onNotiIndiSubscriptionChanged_t cb_onSubscribedIn,
											  cxa_btle_client_cb_onNotiIndiRx_t cb_onRxIn,
											  void* userVarIn);


/**
 * @public
 */
void cxa_btle_client_unsubscribeToNotifications(cxa_btle_client_t *const btlecIn,
												cxa_eui48_t *const targetAddrIn,
												const char *const serviceUuidIn,
												const char *const characteristicUuidIn,
												cxa_btle_client_cb_onNotiIndiSubscriptionChanged_t cb_onUnsubscribedIn,
												void* userVarIn);


/**
 * @protected
 */
void cxa_btle_client_notify_onBecomesReady(cxa_btle_client_t *const btlecIn);


/**
 * @protected
 */
void cxa_btle_client_notify_onFailedInit(cxa_btle_client_t *const btlecIn, bool willAutoRetryIn);


/**
 * @protected
 */
void cxa_btle_client_notify_advertRx(cxa_btle_client_t *const btlecIn, cxa_btle_advPacket_t *packetIn);


/**
 * @protected
 */
void cxa_btle_client_notify_scanStart(cxa_btle_client_t *const btlecIn, bool wasSuccessfulIn);


/**
 * @protected
 */
void cxa_btle_client_notify_scanStop(cxa_btle_client_t *const btlecIn);


/**
 * @protected
 */
void cxa_btle_client_notify_connectionStarted(cxa_btle_client_t *const btlecIn, cxa_eui48_t *const targetAddrIn);


/**
 * @protected
 */
void cxa_btle_client_notify_connectionClose_expected(cxa_btle_client_t *const btlecIn, cxa_eui48_t *const targetAddrIn);


/**
 * @protected
 */
void cxa_btle_client_notify_connectionClose_unexpected(cxa_btle_client_t *const btlecIn, cxa_eui48_t *const targetAddrIn, cxa_btle_client_disconnectReason_t reasonIn);


/**
 * @protected
 */
void cxa_btle_client_notify_writeComplete(cxa_btle_client_t *const btlecIn,
										  cxa_eui48_t *const targetAddrIn,
										  const char *const serviceUuidIn,
										  const char *const characteristicUuidIn,
										  bool wasSuccessfulIn);


/**
 * @protected
 */
void cxa_btle_client_notify_readComplete(cxa_btle_client_t *const btlecIn,
										 cxa_eui48_t *const targetAddrIn,
										 const char *const serviceUuidIn,
										 const char *const characteristicUuidIn,
										 bool wasSuccessfulIn,
										 cxa_fixedByteBuffer_t *fbb_readDataIn);


/**
 * @protected
 */
void cxa_btle_client_notify_notiIndiSubscriptionChanged(cxa_btle_client_t *const btlecIn,
														cxa_eui48_t *const targetAddrIn,
														const char *const serviceUuidIn,
														const char *const characteristicUuidIn,
														bool wasSuccessfulIn,
														bool notificationsEnableIn);


/**
 * @protected
 */
void cxa_btle_client_notify_notiIndiRx(cxa_btle_client_t *const btlecIn,
									   cxa_eui48_t *const targetAddrIn,
									   const char *const serviceUuidIn,
									   const char *const characteristicUuidIn,
									   cxa_fixedByteBuffer_t *fbb_dataIn);

#endif
