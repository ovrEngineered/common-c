/**
 * @file
 * @copyright 2019 opencxa.org
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
#ifndef CXA_SILABSBGAPI_BTLE_CONNECTION_H_
#define CXA_SILABSBGAPI_BTLE_CONNECTION_H_


// ******** includes ********
#include <gecko_bglib.h>
#include <stdbool.h>

#include <cxa_btle_central.h>
#include <cxa_btle_connection.h>
#include <cxa_fixedByteBuffer.h>
#include <cxa_logger_header.h>
#include <cxa_stateMachine.h>



// ******** global macro definitions ********
#ifndef CXA_SILABSBGAPI_BTLE_CONNECTION_MAXNUM_CACHED_SERVICES
	#define CXA_SILABSBGAPI_BTLE_CONNECTION_MAXNUM_CACHED_SERVICES				4
#endif

#ifndef CXA_SILABSBGAPI_BTLE_CONNECTION_MAXNUM_CACHED_CHARACTERISTICS
	#define CXA_SILABSBGAPI_BTLE_CONNECTION_MAXNUM_CACHED_CHARACTERISTICS		8
#endif

#ifndef CXA_SILABSBGAPI_BTLE_CONNECTION_BUFFER_SIZE_BYTES
	#define CXA_SILABSBGAPI_BTLE_CONNECTION_BUFFER_SIZE_BYTES					128
#endif


// ******** global type definitions *********
/**
 * @protected
 */
typedef struct cxa_siLabsBgApi_btle_connection cxa_siLabsBgApi_btle_connection_t;


/**
 * @private
 */
typedef enum
{
	CXA_SILABSBGAPI_PROCTYPE_NONE,
	CXA_SILABSBGAPI_PROCTYPE_READ,
	CXA_SILABSBGAPI_PROCTYPE_WRITE,
	CXA_SILABSBGAPI_PROCTYPE_NOTI_INDI_CHANGE
}cxa_siLabsBgApi_btle_connection_procType_t;


/**
 * @private
 */
typedef struct
{
	cxa_btle_uuid_t uuid;
	uint32_t handle;
}cxa_siLabsBgApi_btle_connection_cachedServiceEntry_t;


/**
 * @private
 */
typedef struct
{
	cxa_siLabsBgApi_btle_connection_cachedServiceEntry_t* service;

	cxa_btle_uuid_t uuid;
	uint16_t handle;
}cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t;


/**
 * @private
 */
struct cxa_siLabsBgApi_btle_connection
{
	cxa_btle_connection_t super;

	uint8_t connHandle;
	bool isRandomAddress;

	cxa_siLabsBgApi_btle_connection_procType_t targetProcType;
	bool procSuccessful;
	cxa_btle_uuid_t targetServiceUuid;
	cxa_btle_uuid_t targetCharacteristicUuid;
	bool procEnableNotifications;

	cxa_array_t cachedServices;
	cxa_siLabsBgApi_btle_connection_cachedServiceEntry_t cachedServices_raw[CXA_SILABSBGAPI_BTLE_CONNECTION_MAXNUM_CACHED_SERVICES];

	cxa_array_t cachedCharacteristics;
	cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t cachedCharacteristics_raw[CXA_SILABSBGAPI_BTLE_CONNECTION_MAXNUM_CACHED_CHARACTERISTICS];

	cxa_fixedByteBuffer_t fbb_write;
	uint8_t fbb_write_raw[CXA_SILABSBGAPI_BTLE_CONNECTION_BUFFER_SIZE_BYTES];

	cxa_fixedByteBuffer_t fbb_read;
	uint8_t fbb_read_raw[CXA_SILABSBGAPI_BTLE_CONNECTION_BUFFER_SIZE_BYTES];

	cxa_btle_connection_disconnectReason_t disconnectReason;

	cxa_stateMachine_t stateMachine;
	cxa_logger_t logger;
};


// ******** global function prototypes ********
void cxa_siLabsBgApi_btle_connection_init(cxa_siLabsBgApi_btle_connection_t *const connIn, cxa_btle_central_t *const parentClientIn, int threadIdIn);

bool cxa_siLabsBgApi_btle_connection_isUsed(cxa_siLabsBgApi_btle_connection_t *const connIn);

void cxa_siLabsBgApi_btle_connection_startConnection(cxa_siLabsBgApi_btle_connection_t *const connIn, cxa_eui48_t *const targetAddrIn, bool isRandomAddrIn);

void cxa_siLabsBgApi_btle_connection_handleEvent_opened(cxa_siLabsBgApi_btle_connection_t *const connIn);
void cxa_siLabsBgApi_btle_connection_handleEvent_closed(cxa_siLabsBgApi_btle_connection_t *const connIn, uint16_t reasonCodeIn);
void cxa_siLabsBgApi_btle_connection_handleEvent_serviceResolved(cxa_siLabsBgApi_btle_connection_t *const connIn, cxa_btle_uuid_t *const uuid, uint32_t handleIn);
void cxa_siLabsBgApi_btle_connection_handleEvent_characteristicResolved(cxa_siLabsBgApi_btle_connection_t *const connIn, cxa_btle_uuid_t *const uuid, uint16_t handleIn);
void cxa_siLabsBgApi_btle_connection_handleEvent_characteristicValueUpdated(cxa_siLabsBgApi_btle_connection_t *const connIn, uint16_t handleIn, enum gatt_att_opcode opcodeIn, uint8_t *const dataIn, size_t dataLen_bytesIn);
void cxa_siLabsBgApi_btle_connection_handleEvent_procedureComplete(cxa_siLabsBgApi_btle_connection_t *const connIn, uint16_t resultCodeIn);

#endif
