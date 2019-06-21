/*
 * Copyright 2019 ovrEngineered, LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining 
 * a copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
 * copies of the Software, and to permit persons to whom the Software is furnished to 
 * do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all 
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_BTLE_PERIPHERAL_H_
#define CXA_BTLE_PERIPHERAL_H_


// ******** includes ********
#include <cxa_array.h>
#include <cxa_btle_central.h>
#include <cxa_btle_uuid.h>
#include <cxa_config.h>
#include <cxa_eui48.h>
#include <cxa_logger_header.h>


// ******** global macro definitions ********
#ifndef CXA_BTLE_PERIPHERAL_MAXNUM_LISTENERS
	#define CXA_BTLE_PERIPHERAL_MAXNUM_LISTENERS				2
#endif

#ifndef CXA_BTLE_PERIPHERAL_MAXNUM_CHAR_ENTRIES
	#define CXA_BTLE_PERIPHERAL_MAXNUM_CHAR_ENTRIES				16
#endif

#ifndef CXA_BTLE_PERIPHERAL_MAXNUM_DEFERRED_OPERATIONS
	#define CXA_BTLE_PERIPHERAL_MAXNUM_DEFERRED_OPERATIONS		4
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_btle_peripheral cxa_btle_peripheral_t;


/**
 * @public
 */
typedef struct cxa_btle_peripheral_deferredOperationEntry cxa_btle_peripheral_deferredOperationEntry_t;


/**
 * @public
 */
typedef enum
{
	CXA_BTLE_PERIPHERAL_READRET_SUCCESS = 0x0000,
	CXA_BTLE_PERIPHERAL_READRET_NOT_PERMITTED = 0x0403,
	CXA_BTLE_PERIPHERAL_READRET_VALUE_NOT_ALLOWED = 0x0413,
	CXA_BTLE_PERIPHERAL_READRET_ATTRIBUTE_NOT_FOUND = 0x040a,
	CXA_BTLE_PERIPHERAL_READRET_UNLIKELY = 0x040e
}cxa_btle_peripheral_readRetVal_t;


/**
 * @public
 */
typedef enum
{
	CXA_BTLE_PERIPHERAL_WRITERET_SUCCESS = 0x0000,
	CXA_BTLE_PERIPHERAL_WRITERET_NOT_PERMITTED = 0x0403,
	CXA_BTLE_PERIPHERAL_WRITERET_VALUE_NOT_ALLOWED = 0x0413,
	CXA_BTLE_PERIPHERAL_WRITERET_ATTRIBUTE_NOT_FOUND = 0x040a,
	CXA_BTLE_PERIPHERAL_WRITERET_UNLIKELY = 0x040e
}cxa_btle_peripheral_writeRetVal_t;


/**
 * @public
 */
typedef void (*cxa_btle_peripheral_cb_onReady_t)(cxa_btle_peripheral_t *const btlepIn, void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_peripheral_cb_onFailedInit_t)(cxa_btle_peripheral_t *const btlepIn, bool willAutoRetryIn, void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_peripheral_cb_onConnectionOpened_t)(cxa_eui48_t *const sourceAddrIn, void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_btle_peripheral_cb_onConnectionClosed_t)(cxa_eui48_t *const sourceAddrIn, void* userVarIn);


/**
 * @public
 */
typedef cxa_btle_peripheral_readRetVal_t (*cxa_btle_peripheral_cb_onReadRequest_t)(cxa_fixedByteBuffer_t *const fbbDataOut, void *userVarIn);


/**
 * @public
 */
typedef cxa_btle_peripheral_readRetVal_t (*cxa_btle_peripheral_cb_onDeferredReadRequest_t)(cxa_btle_peripheral_deferredOperationEntry_t* doeIn, void *userVarIn);


/**
 * @public
 */
typedef cxa_btle_peripheral_writeRetVal_t (*cxa_btle_peripheral_cb_onWriteRequest_t)(cxa_fixedByteBuffer_t *const fbbDataIn, void *userVarIn);


/**
 * @public
 */
typedef cxa_btle_peripheral_writeRetVal_t (*cxa_btle_peripheral_cb_onDeferredWriteRequest_t)(cxa_btle_peripheral_deferredOperationEntry_t* doeIn, cxa_fixedByteBuffer_t *const fbbDataIn, void *userVarIn);


/**
 * @protected
 */
typedef void (*cxa_btle_peripheral_scm_sendNotification_t)(cxa_btle_peripheral_t *const superIn, const char *const serviceUuidStrIn, const char *const characteristicUuidStrIn, cxa_fixedByteBuffer_t *const fbb_dataIn);


/**
 * @protected
 */
typedef void (*cxa_btle_peripheral_scm_sendDeferredReadResponse_t)(cxa_btle_peripheral_t *const superIn, cxa_eui48_t *const sourceAddrIn, const char *const serviceUuidStrIn, const char *const characteristicUuidStrIn, cxa_btle_peripheral_readRetVal_t retValIn, cxa_fixedByteBuffer_t *const fbbReadDataIn);


/**
 * @protected
 */
typedef void (*cxa_btle_peripheral_scm_sendDeferredWriteResponse_t)(cxa_btle_peripheral_t *const superIn, cxa_eui48_t *const sourceAddrIn, const char *const serviceUuidStrIn, const char *const characteristicUuidStrIn, cxa_btle_peripheral_writeRetVal_t retValIn);


/**
 * @protected
 */
typedef void (*cxa_btle_peripheral_scm_setAdvertisingInfo_t)(cxa_btle_peripheral_t *const superIn, uint32_t advertPeriod_msIn, cxa_fixedByteBuffer_t *const fbbAdvertDataIn);


/**
 * @protected
 */
typedef void (*cxa_btle_peripheral_scm_startAdvertising_t)(cxa_btle_peripheral_t *const superIn);


/**
 * @private
 */
typedef struct
{
	cxa_btle_peripheral_cb_onReady_t cb_onReady;
	cxa_btle_peripheral_cb_onFailedInit_t cb_onFailedInit;
	cxa_btle_peripheral_cb_onConnectionOpened_t cb_onConnectionOpened;
	cxa_btle_peripheral_cb_onConnectionClosed_t cb_onConnectionClosed;
	void* userVar;
}cxa_btle_peripheral_listener_entry_t;


/**
 * @private
 */
typedef struct
{
	cxa_btle_uuid_t serviceUuid;
	cxa_btle_uuid_t charUuid;

	struct
	{
		cxa_btle_peripheral_cb_onReadRequest_t onReadRequest;
		cxa_btle_peripheral_cb_onDeferredReadRequest_t onDeferredReadRequest;
		void* userVar_read;

		cxa_btle_peripheral_cb_onWriteRequest_t onWriteRequest;
		cxa_btle_peripheral_cb_onDeferredWriteRequest_t onDeferredWriteRequest;
		void* userVar_write;
	}cbs;

}cxa_btle_peripheral_charEntry_t;


/**
 * @private
 */
struct cxa_btle_peripheral_deferredOperationEntry
{
	bool isInUse;

	cxa_eui48_t sourceMac;
	cxa_btle_uuid_t serviceUuid;
	cxa_btle_uuid_t charUuid;
};


/**
 * @private
 */
struct cxa_btle_peripheral
{
	struct
	{
		cxa_btle_peripheral_scm_sendNotification_t sendNotification;
		cxa_btle_peripheral_scm_sendDeferredReadResponse_t sendDeferredReadResponse;
		cxa_btle_peripheral_scm_sendDeferredWriteResponse_t sendDeferredWriteResponse;
		cxa_btle_peripheral_scm_setAdvertisingInfo_t setAdvertisingInfo;
		cxa_btle_peripheral_scm_startAdvertising_t startAdvertising;
	}scms;

	cxa_array_t charEntries;
	cxa_btle_peripheral_charEntry_t charEntries_raw[CXA_BTLE_PERIPHERAL_MAXNUM_CHAR_ENTRIES];

	cxa_array_t listeners;
	cxa_btle_peripheral_listener_entry_t listeners_raw[CXA_BTLE_PERIPHERAL_MAXNUM_LISTENERS];

	cxa_btle_peripheral_deferredOperationEntry_t deferredOperations[CXA_BTLE_PERIPHERAL_MAXNUM_DEFERRED_OPERATIONS];

	cxa_logger_t logger;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_btle_peripheral_init(cxa_btle_peripheral_t *const btlepIn,
							  cxa_btle_peripheral_scm_sendNotification_t scm_sendNotificationIn,
							  cxa_btle_peripheral_scm_sendDeferredReadResponse_t scm_sendDeferredReadResponseIn,
							  cxa_btle_peripheral_scm_sendDeferredWriteResponse_t scm_sendDeferredWriteResponseIn,
							  cxa_btle_peripheral_scm_setAdvertisingInfo_t scm_setAdvertisingInfoIn,
							  cxa_btle_peripheral_scm_startAdvertising_t scm_startAdvertisingIn);


/**
 * @public
 */
void cxa_btle_peripheral_addListener(cxa_btle_peripheral_t *const btlepIn,
									 cxa_btle_peripheral_cb_onReady_t cb_onReadyIn,
									 cxa_btle_peripheral_cb_onFailedInit_t cb_onFailedInitIn,
									 cxa_btle_peripheral_cb_onConnectionOpened_t cb_onConnectionOpenedIn,
									 cxa_btle_peripheral_cb_onConnectionClosed_t cb_onConnectionClosedIn,
									 void* userVarIn);


/**
 * @public
 */
void cxa_btle_peripheral_registerCharacteristicHandler_read(cxa_btle_peripheral_t *const btlepIn,
															const char *const serviceUuidStrIn,
															const char *const charUuidStrIn,
															cxa_btle_peripheral_cb_onReadRequest_t cb_onReadIn, void *userVarIn);

/**
 * @public
 */
void cxa_btle_peripheral_registerCharacteristicHandler_deferredRead(cxa_btle_peripheral_t *const btlepIn,
																	const char *const serviceUuidStrIn,
																	const char *const charUuidStrIn,
																	cxa_btle_peripheral_cb_onDeferredReadRequest_t cb_onReadIn, void* userVarIn);

/**
 * @public
 */
void cxa_btle_peripheral_registerCharacteristicHandler_write(cxa_btle_peripheral_t *const btlepIn,
															 const char *const serviceUuidStrIn,
															 const char *const charUuidStrIn,
															 cxa_btle_peripheral_cb_onWriteRequest_t cb_onWriteIn, void *userVarIn);


/**
 * @public
 */
void cxa_btle_peripheral_registerCharacteristicHandler_deferredWrite(cxa_btle_peripheral_t *const btlepIn,
		 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 const char *const serviceUuidStrIn,
																	 const char *const charUuidStrIn,
																	 cxa_btle_peripheral_cb_onDeferredWriteRequest_t cb_onWriteIn, void* userVarIn);


/**
 * @public
 */
void cxa_btle_peripheral_setAdvertisingInfo(cxa_btle_peripheral_t *const btlepIn,
										  	uint32_t advertPeriod_msIn,
											cxa_fixedByteBuffer_t *const fbbAdvertDataIn);


/**
 * @public
 */
void cxa_btle_peripheral_sendNotification(cxa_btle_peripheral_t *const btlepIn,
										  const char *const serviceUuidStrIn,
										  const char *const charUuidStrIn,
										  void *const dataIn,
										  size_t numBytesIn);


/**
 * @public
 */
void cxa_btle_peripheral_sendNotification_fbb(cxa_btle_peripheral_t *const btlepIn,
											  const char *const serviceUuidStrIn,
											  const char *const charUuidStrIn,
											  cxa_fixedByteBuffer_t *const fbb_dataIn);


/**
 * @public
 */
void cxa_btle_peripheral_completeDeferredRead(cxa_btle_peripheral_t *const btlepIn,
											  cxa_btle_peripheral_deferredOperationEntry_t* doeIn,
											  cxa_btle_peripheral_readRetVal_t retValIn,
											  cxa_fixedByteBuffer_t *const fbbReadDataIn);


/**
 * @public
 */
void cxa_btle_peripheral_completeDeferredWrite(cxa_btle_peripheral_t *const btlepIn,
											   cxa_btle_peripheral_deferredOperationEntry_t* doeIn,
											   cxa_btle_peripheral_writeRetVal_t retValIn);


/**
 * @protected
 */
void cxa_btle_peripheral_notify_onBecomesReady(cxa_btle_peripheral_t *const btlepIn);


/**
 * @protected
 */
void cxa_btle_peripheral_notify_onFailedInit(cxa_btle_peripheral_t *const btlepIn, bool willAutoRetryIn);


/**
 * @protected
 */
void cxa_btle_peripheral_notify_connectionOpened(cxa_btle_peripheral_t *const btlepIn, cxa_eui48_t *const targetAddrIn);


/**
 * @protected
 */
void cxa_btle_peripheral_notify_connectionClosed(cxa_btle_peripheral_t *const btlepIn, cxa_eui48_t *const targetAddrIn);


/**
 * @protected
 * @return true if an immediate response should be send, false to defer
 */
bool cxa_btle_peripheral_notify_readRequest(cxa_btle_peripheral_t *const btlepIn,
											cxa_eui48_t *const sourceMacAddrIn,
											cxa_btle_uuid_t *const serviceUuidIn,
											cxa_btle_uuid_t *const charUuidIn,
											cxa_fixedByteBuffer_t *const dataOut,
											cxa_btle_peripheral_readRetVal_t *const retValOut);


/**
 * @protected
 * @return true if an immediate response should be send, false to defer
 */
bool cxa_btle_peripheral_notify_writeRequest(cxa_btle_peripheral_t *const btlepIn,
											 cxa_eui48_t *const sourceMacAddrIn,
											 cxa_btle_uuid_t *const serviceUuidIn,
											 cxa_btle_uuid_t *const charUuidIn,
											 cxa_fixedByteBuffer_t *const dataIn,
											 cxa_btle_peripheral_writeRetVal_t *const retValOut);

#endif
