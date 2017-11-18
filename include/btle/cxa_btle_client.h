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
#include <cxa_btle_uuid.h>
#include <cxa_eui48.h>
#include <cxa_fixedByteBuffer.h>
#include <cxa_uuid128.h>


// ******** global macro definitions ********
#ifndef CXA_BTLE_CLIENT_MAXNUM_LISTENERS
	#define CXA_BTLE_CLIENT_MAXNUM_LISTENERS		2
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
	CXA_BTLE_ADVFIELDTYPE_FLAGS = 0x01,
	CXA_BTLE_ADVFIELDTYPE_TXPOWER = 0x0A,
	CXA_BTLE_ADVFIELDTYPE_MAN_DATA = 0xFF
}cxa_btle_advFieldType_t;


/**
 * @public
 */
typedef enum
{
	CXA_BTLE_CLIENT_STATE_STARTUP,
	CXA_BTLE_CLIENT_STATE_STARTUPFAILED,
	CXA_BTLE_CLIENT_STATE_READY,
	CXA_BTLE_CLIENT_STATE_SCANNING,
	CXA_BTLE_CLIENT_STATE_CONNECTING,
	CXA_BTLE_CLIENT_STATE_CONNECTED
}cxa_btle_client_state_t;


/**
 * @public
 */
typedef struct
{
	uint8_t length;
	cxa_btle_advFieldType_t type;

	union
	{
		struct
		{
			uint8_t flags;
		}asFlags;

		struct
		{
			int8_t txPower_dBm;
		}asTxPower;

		struct
		{
			uint16_t companyId;
			cxa_fixedByteBuffer_t manBytes;
		}asManufacturerData;
	};
}cxa_btle_advField_t;


/**
 * @public
 */
typedef struct
{
	cxa_eui48_t addr;
	bool isRandomAddress;
	int rssi;

	cxa_array_t advFields;
}cxa_btle_advPacket_t;


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
typedef void (*cxa_btle_client_cb_onConnectionOpened_t)(bool wasSuccessfulIn, void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_client_cb_onConnectionClosed_t)(void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_client_cb_onWriteComplete_t)(cxa_btle_uuid_t *const uuid_serviceIn,
													 cxa_btle_uuid_t *const uuid_charIn,
													 bool wasSuccessfulIn, void* userVarIn);


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
typedef void (*cxa_btle_client_scm_stopConnection_t)(cxa_btle_client_t *const superIn);


/**
 * @private
 */
typedef bool (*cxa_btle_client_scm_isConnected_t)(cxa_btle_client_t *const superIn);


/**
 * @private
 */
typedef void (*cxa_btle_client_scm_writeToCharacteristic_t)(cxa_btle_client_t *const superIn,
															const char *const serviceIdIn,
															const char *const characteristicIdIn,
															cxa_fixedByteBuffer_t *const dataIn);


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
struct cxa_btle_client
{
	cxa_array_t listeners;
	cxa_btle_client_listener_entry_t listeners_raw[CXA_BTLE_CLIENT_MAXNUM_LISTENERS];

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
			cxa_btle_client_cb_onConnectionClosed_t onConnectionClosed;
			void* userVar;
		}connecting;

		struct
		{
			cxa_btle_client_cb_onWriteComplete_t onWriteComplete;
			void* userVar;
		}writing;
	}cbs;

	struct
	{
		cxa_btle_client_scm_getState_t getState;

		cxa_btle_client_scm_startScan_t startScan;
		cxa_btle_client_scm_stopScan_t stopScan;

		cxa_btle_client_scm_startConnection_t startConnection;
		cxa_btle_client_scm_stopConnection_t stopConnection;
		cxa_btle_client_scm_isConnected_t isConnected;

		cxa_btle_client_scm_writeToCharacteristic_t writeToCharacteristic;
	}scms;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_btle_client_init(cxa_btle_client_t *const btlecIn,
						  cxa_btle_client_scm_getState_t scm_getState,
						  cxa_btle_client_scm_startScan_t scm_startScanIn,
						  cxa_btle_client_scm_stopScan_t scm_stopScanIn,
						  cxa_btle_client_scm_startConnection_t scm_startConnectionIn,
						  cxa_btle_client_scm_stopConnection_t scm_stopConnectionIn,
						  cxa_btle_client_scm_isConnected_t scm_isConnectedIn,
						  cxa_btle_client_scm_writeToCharacteristic_t scm_writeToCharIn);


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
									 cxa_btle_client_cb_onConnectionClosed_t cb_connectionUnintentionallyClosedIn,
									 void* userVarIn);


/**
 * @public
 */
void cxa_btle_client_stopConnection(cxa_btle_client_t *const btlecIn,
									cxa_btle_client_cb_onConnectionClosed_t cb_connectionClosedIn,
									void *userVarIn);


/**
 * @public
 */
void cxa_btle_client_writeToCharacteristic_fbb(cxa_btle_client_t *const btlecIn,
										   	   const char *const serviceIdIn,
											   const char *const characteristicIdIn,
											   cxa_fixedByteBuffer_t *const dataIn,
											   cxa_btle_client_cb_onWriteComplete_t cb_writeCompleteIn,
											   void* userVarIn);


/**
 * @public
 */
void cxa_btle_client_writeToCharacteristic(cxa_btle_client_t *const btlecIn,
										   const char *const serviceIdIn,
										   const char *const characteristicIdIn,
										   void *const dataIn,
										   size_t numBytesIn,
										   cxa_btle_client_cb_onWriteComplete_t cb_writeCompleteIn,
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
void cxa_btle_client_notify_connectionStarted(cxa_btle_client_t *const btlecIn, bool wasSuccessfulIn);


/**
 * @protected
 */
void cxa_btle_client_notify_connectionClose(cxa_btle_client_t *const btlecIn);


/**
 * @protected
 */
void cxa_btle_client_notify_writeComplete(cxa_btle_client_t *const btlecIn,
										  cxa_btle_uuid_t *const uuid_serviceIn,
										  cxa_btle_uuid_t *const uuid_charIn,
										  bool wasSuccessfulIn);


/**
 * @protected
 */
void cxa_btle_client_notify_readComplete(cxa_btle_client_t *const btlecIn,
		  	  	  	  	  	  	  	     cxa_btle_uuid_t *const uuid_serviceIn,
										 cxa_btle_uuid_t *const uuid_charIn,
										 bool wasSuccessfulIn);


/**
 * @protected
 */
bool cxa_btle_client_countAdvFields(uint8_t *const bytesIn, size_t maxLen_bytesIn, size_t *const numAdvFieldsOut);


/**
 * @protected
 */
bool cxa_btle_client_parseAdvFieldsForPacket(cxa_btle_advPacket_t *packetIn, size_t numAdvFieldsIn, uint8_t *const bytesIn, size_t maxLen_bytesIn);

#endif
