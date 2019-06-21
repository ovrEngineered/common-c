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
#ifndef CXA_BTLE_CENTRAL_H_
#define CXA_BTLE_CENTRAL_H_


// ******** includes ********
#include <stdbool.h>
#include <stdint.h>

#include <cxa_array.h>
#include <cxa_btle_advPacket.h>
#include <cxa_btle_connection.h>
#include <cxa_btle_uuid.h>
#include <cxa_eui48.h>
#include <cxa_fixedByteBuffer.h>
#include <cxa_logger_header.h>
#include <cxa_uuid128.h>


// ******** global macro definitions ********
#ifndef CXA_BTLE_CENTRAL_MAXNUM_LISTENERS
	#define CXA_BTLE_CENTRAL_MAXNUM_LISTENERS				2
#endif

#ifndef CXA_BTLE_CENTRAL_MAXNUM_CONNECTIONS
	#define CXA_BTLE_CENTRAL_MAXNUM_CONNECTIONS				2
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_btle_central cxa_btle_central_t;


/**
 * @public
 */
typedef enum
{
	CXA_BTLE_CENTRAL_STATE_STARTUP,
	CXA_BTLE_CENTRAL_STATE_STARTUPFAILED,
	CXA_BTLE_CENTRAL_STATE_READY,
}cxa_btle_central_state_t;


/**
 * @public
 */
typedef void (*cxa_btle_central_cb_onReady_t)(cxa_btle_central_t *const btlecIn, void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_central_cb_onFailedInit_t)(cxa_btle_central_t *const btlecIn, bool willAutoRetryIn, void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_central_cb_onAdvertRx_t)(cxa_btle_advPacket_t* packetIn, void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_central_cb_onScanResponseRx_t)(void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_central_cb_onScanStart_t)(bool wasSuccessfulIn, void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_central_cb_onScanStop_t)(void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_central_cb_onConnectionOpened_t)(bool wasSuccessfulIn, cxa_btle_connection_t *const connectionIn, void* userVarIn);


/**
 * @private
 */
typedef cxa_btle_central_state_t (*cxa_btle_central_scm_getState_t)(cxa_btle_central_t *const superIn);


/**
 * @private
 */
typedef void (*cxa_btle_central_scm_startScan_t)(cxa_btle_central_t *const superIn, bool isActiveIn);


/**
 * @private
 */
typedef void (*cxa_btle_central_scm_stopScan_t)(cxa_btle_central_t *const superIn);


/**
 * @private
 */
typedef void (*cxa_btle_central_scm_startConnection_t)(cxa_btle_central_t *const superIn, cxa_eui48_t *const addrIn, bool isRandomAddrIn);


/**
 * @private
 */
typedef struct
{
	cxa_btle_central_cb_onReady_t cb_onReady;
	cxa_btle_central_cb_onFailedInit_t cb_onFailedInit;
	void* userVar;
}cxa_btle_central_listener_entry_t;


/**
 * @private
 */
struct cxa_btle_central
{
	cxa_array_t listeners;
	cxa_btle_central_listener_entry_t listeners_raw[CXA_BTLE_CENTRAL_MAXNUM_LISTENERS];

	bool hasActivityAvailable;

	struct
	{
		struct
		{
			cxa_btle_central_cb_onAdvertRx_t onAdvert;
			cxa_btle_central_cb_onScanResponseRx_t onScanResp;
			cxa_btle_central_cb_onScanStart_t onScanStart;
			cxa_btle_central_cb_onScanStop_t onScanStop;
			void *userVar;
		}scanning;

		struct
		{
			cxa_btle_central_cb_onConnectionOpened_t onConnectionOpened;
			void* userVar;
		}connecting;
	}cbs;

	struct
	{
		cxa_btle_central_scm_getState_t getState;

		cxa_btle_central_scm_startScan_t startScan;
		cxa_btle_central_scm_stopScan_t stopScan;

		cxa_btle_central_scm_startConnection_t startConnection;
	}scms;

	cxa_logger_t logger;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_btle_central_init(cxa_btle_central_t *const btlecIn,
						   cxa_btle_central_scm_getState_t scm_getStateIn,
						   cxa_btle_central_scm_startScan_t scm_startScanIn,
						   cxa_btle_central_scm_stopScan_t scm_stopScanIn,
						   cxa_btle_central_scm_startConnection_t scm_startConnectionIn);


/**
 * @public
 */
void cxa_btle_central_addListener(cxa_btle_central_t *const btlecIn,
								  cxa_btle_central_cb_onReady_t cb_onReadyIn,
								  cxa_btle_central_cb_onFailedInit_t cb_onFailedInitIn,
								  void* userVarIn);


/**
 * @public
 */
void cxa_btle_central_startScan_passive(cxa_btle_central_t *const btlecIn,
									    cxa_btle_central_cb_onScanStart_t cb_scanStartIn,
									    cxa_btle_central_cb_onAdvertRx_t cb_advIn,
									    void* userVarIn);


/**
 * @public
 */
void cxa_btle_central_startScan_active(cxa_btle_central_t *const btlecIn,
									   cxa_btle_central_cb_onScanStart_t cb_scanStartIn,
									   cxa_btle_central_cb_onAdvertRx_t cb_advIn,
									   cxa_btle_central_cb_onScanResponseRx_t cb_scanRespIn,
									   void* userVarIn);


/**
 * @public
 */
void cxa_btle_central_stopScan(cxa_btle_central_t *const btlecIn,
						 	   cxa_btle_central_cb_onScanStop_t cb_scanStopIn,
							   void* userVarIn);


/**
 * @public
 */
cxa_btle_central_state_t cxa_btle_central_getState(cxa_btle_central_t *const btlecIn);


/**
 * @public
 */
bool cxa_btle_central_hasActivityAvailable(cxa_btle_central_t *const btlecIn);


/**
 * @public
 */
void cxa_btle_central_startConnection(cxa_btle_central_t *const btlecIn,
									  cxa_eui48_t *const addrIn,
									  bool isRandomAddrIn,
									  cxa_btle_central_cb_onConnectionOpened_t cb_connectionOpenedIn,
									  void* userVarIn);


/**
 * @protected
 */
void cxa_btle_central_notify_onBecomesReady(cxa_btle_central_t *const btlecIn);


/**
 * @protected
 */
void cxa_btle_central_notify_onFailedInit(cxa_btle_central_t *const btlecIn,
										  bool willAutoRetryIn);


/**
 * @protected
 */
void cxa_btle_central_notify_advertRx(cxa_btle_central_t *const btlecIn,
									  cxa_btle_advPacket_t *packetIn);


/**
 * @protected
 */
void cxa_btle_central_notify_scanStart(cxa_btle_central_t *const btlecIn,
									   bool wasSuccessfulIn);


/**
 * @protected
 */
void cxa_btle_central_notify_scanStop(cxa_btle_central_t *const btlecIn);


/**
 * @protected
 */
void cxa_btle_central_notify_connectionStarted(cxa_btle_central_t *const btlecIn,
											   bool wasSuccessfulIn,
											   cxa_btle_connection_t *const connIn);


#endif
