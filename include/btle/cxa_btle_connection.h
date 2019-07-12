/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_BTLE_CONNECTION_H_
#define CXA_BTLE_CONNECTION_H_


// ******** includes ********
#include <cxa_btle_uuid.h>
#include <cxa_eui48.h>
#include <cxa_fixedByteBuffer.h>


// ******** global macro definitions ********
#ifndef CXA_BTLE_CONNECTION_MAXNUM_NOTIINDI_SUBSCRIPTIONS
	#define CXA_BTLE_CONNECTION_MAXNUM_NOTIINDI_SUBSCRIPTIONS	2
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_btle_connection cxa_btle_connection_t;


/**
 * @public
 */
typedef enum
{
	CXA_BTLE_CONNECTION_DISCONNECT_REASON_USER_REQUESTED,
	CXA_BTLE_CONNECTION_DISCONNECT_REASON_CONNECTION_TIMEOUT,
	CXA_BTLE_CONNECTION_DISCONNECT_REASON_STACK,
	CXA_BTLE_CONNECTION_DISCONNECT_REASON_BAD_STATE
}cxa_btle_connection_disconnectReason_t;


/**
 * @public
 */
typedef void (*cxa_btle_connection_cb_onConnectionClosed_t)(cxa_btle_connection_disconnectReason_t reasonIn,
															void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_connection_cb_onReadComplete_t)(bool wasSuccessfulIn,
														cxa_fixedByteBuffer_t *fbb_readDataIn,
														void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_connection_cb_onWriteComplete_t)(bool wasSuccessfulIn, void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_connection_cb_onNotiIndiSubscriptionChanged_t)(const char *const serviceUuidIn,
																	   const char *const characteristicUuidIn,
																	   bool wasSuccessfulIn,
																	   void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_connection_cb_onNotiIndiRx_t)(const char *const serviceUuidIn,
													  const char *const characteristicUuidIn,
													  cxa_fixedByteBuffer_t *fbb_readDataIn,
													  void* userVarIn);


/**
 * @protected
 */
typedef void (*cxa_btle_connection_scm_stopConnection_t)(cxa_btle_connection_t *const superIn);


/**
 * @protected
 */
typedef void (*cxa_btle_connection_scm_readFromCharacteristic_t)(cxa_btle_connection_t *const superIn,
																 const char *const serviceUuidIn,
																 const char *const characteristicUuidIn);


/**
 * @protected
 */
typedef void (*cxa_btle_connection_scm_writeToCharacteristic_t)(cxa_btle_connection_t *const superIn,
																const char *const serviceUuidIn,
																const char *const characteristicUuidIn,
																cxa_fixedByteBuffer_t *const dataIn);

/**
 * @protected
 */
typedef void (*cxa_btle_connection_scm_changeNotifications_t)(cxa_btle_connection_t *const superIn,
															  const char *const serviceUuidIn,
															  const char *const characteristicUuidIn,
															  bool enableNotificationsIn);


/**
 * @private
 */
typedef struct
{
	cxa_btle_uuid_t uuid_service;
	cxa_btle_uuid_t uuid_characteristic;

	cxa_btle_connection_cb_onNotiIndiRx_t cb_onRx;
	void* userVar;
}cxa_btle_connection_notiIndiSubscription_t;


/**
 * @private
 */
struct cxa_btle_connection
{
	void* btlec;				// cxa_btle_central_t
	cxa_eui48_t targetAddr;

	cxa_array_t notiIndiSubs;
	cxa_btle_connection_notiIndiSubscription_t notiIndiSubs_raw[CXA_BTLE_CONNECTION_MAXNUM_NOTIINDI_SUBSCRIPTIONS];

	struct
	{
		struct
		{
			cxa_btle_connection_cb_onConnectionClosed_t func;
			void* userVar;
		}connectionClosed;

		struct
		{
			cxa_btle_connection_cb_onReadComplete_t func;
			void* userVar;
		}readFromChar;

		struct
		{
			cxa_btle_connection_cb_onWriteComplete_t func;
			void* userVar;
		}writeToChar;

		struct
		{
			cxa_btle_connection_cb_onNotiIndiSubscriptionChanged_t func;
			void* userVar;
		}subscribeToChar;

		struct
		{
			cxa_btle_connection_cb_onNotiIndiSubscriptionChanged_t func;
			void* userVar;
		}unsubscribeFromChar;
	}cbs;

	struct
	{
		cxa_btle_connection_scm_stopConnection_t stopConnection;

		cxa_btle_connection_scm_readFromCharacteristic_t readFromCharacteristic;
		cxa_btle_connection_scm_writeToCharacteristic_t writeToCharacteristic;

		cxa_btle_connection_scm_changeNotifications_t changeNotifications;
	}scms;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_btle_connection_init(cxa_btle_connection_t *const connIn,
							  void *const btlecIn,
							  cxa_btle_connection_scm_stopConnection_t scm_stopConnectionIn,
							  cxa_btle_connection_scm_readFromCharacteristic_t scm_readFromCharIn,
							  cxa_btle_connection_scm_writeToCharacteristic_t scm_writeToCharIn,
							  cxa_btle_connection_scm_changeNotifications_t scm_changeNotiIndisIn);


/**
 * @protected
 */
void cxa_btle_connection_setTargetAddress(cxa_btle_connection_t *const connIn,
										  cxa_eui48_t *const targetAddrIn);


/**
 * @public
 */
void cxa_btle_connection_setOnClosedCb(cxa_btle_connection_t *const connIn,
									   cxa_btle_connection_cb_onConnectionClosed_t cbIn,
									   void* userVarIn);


/**
 * @public
 */
cxa_eui48_t* cxa_btle_connection_getTargetMacAddress(cxa_btle_connection_t *const connIn);


/**
 * @public
 */
void cxa_btle_connection_readFromCharacteristic(cxa_btle_connection_t *const connIn,
												const char *const serviceUuidIn,
												const char *const characteristicUuidIn,
												cxa_btle_connection_cb_onReadComplete_t cbIn,
												void* userVarIn);


/**
 * @public
 */
void cxa_btle_connection_writeToCharacteristic(cxa_btle_connection_t *const connIn,
											   const char *const serviceUuidIn,
											   const char *const characteristicUuidIn,
											   cxa_fixedByteBuffer_t *const dataIn,
											   cxa_btle_connection_cb_onWriteComplete_t cbIn,
											   void *userVarIn);


/**
 * @public
 */
void cxa_btle_connection_writeToCharacteristic_ptr(cxa_btle_connection_t *const connIn,
											   	   const char *const serviceUuidIn,
												   const char *const characteristicUuidIn,
												   void *const dataIn,
												   size_t numBytesIn,
												   cxa_btle_connection_cb_onWriteComplete_t cbIn,
												   void *userVarIn);


/**
 * @public
 */
void cxa_btle_connection_subscribeToNotifications(cxa_btle_connection_t *const connIn,
												  const char *const serviceUuidIn,
												  const char *const characteristicUuidIn,
												  cxa_btle_connection_cb_onNotiIndiSubscriptionChanged_t cb_onSubscribedIn,
												  cxa_btle_connection_cb_onNotiIndiRx_t cb_onRxIn,
												  void* userVarIn);


/**
 * @public
 */
void cxa_btle_connection_unsubscribeToNotifications(cxa_btle_connection_t *const connIn,
													const char *const serviceUuidIn,
													const char *const characteristicUuidIn,
													cxa_btle_connection_cb_onNotiIndiSubscriptionChanged_t cb_onUnsubscribedIn,
													void* userVarIn);

/**
 * @public
 */
void cxa_btle_connection_stop(cxa_btle_connection_t *const connIn);


/**
 * @protected
 */
void cxa_btle_connection_notify_connectionClose(cxa_btle_connection_t *const connIn,
												cxa_btle_connection_disconnectReason_t reasonIn);


/**
 * @protected
 */
void cxa_btle_connection_notify_writeComplete(cxa_btle_connection_t *const connIn,
											  const char *const serviceUuidIn,
											  const char *const characteristicUuidIn,
											  bool wasSuccessfulIn);


/**
 * @protected
 */
void cxa_btle_connection_notify_readComplete(cxa_btle_connection_t *const connIn,
											 const char *const serviceUuidIn,
											 const char *const characteristicUuidIn,
											 bool wasSuccessfulIn,
											 cxa_fixedByteBuffer_t *fbb_readDataIn);


/**
 * @protected
 */
void cxa_btle_connection_notify_notiIndiSubscriptionChanged(cxa_btle_connection_t *const connIn,
															const char *const serviceUuidIn,
															const char *const characteristicUuidIn,
															bool wasSuccessfulIn,
															bool notificationsEnableIn);

/**
 * @protected
 */
void cxa_btle_connection_notify_notiIndiRx(cxa_btle_connection_t *const connIn,
										   const char *const serviceUuidIn,
										   const char *const characteristicUuidIn,
										   cxa_fixedByteBuffer_t *fbb_dataIn);

#endif
