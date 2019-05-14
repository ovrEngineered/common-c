/**
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
#include "cxa_siLabsBgApi_btle_connection.h"


// ******** includes ********
#include <gecko_bglib.h>

#include <cxa_assert.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define	CONNECT_TIMEOUT_MS				5000
#define	PROCEDURE_TIMEOUT_MS			5000

#define DISCONNECT_TIMEOUT_MS			2000


// ******** local type definitions ********
typedef enum
{
	STATE_UNUSED,
	STATE_CONNECTING,
	STATE_CONNECTING_TIMEOUT,
	STATE_CONNECTED_IDLE,
	STATE_CONNECTED_RESOLVE_SERVICE,
	STATE_CONNECTED_RESOLVE_CHAR,
	STATE_CONNECTED_PROCEDURE_TIMEOUT,
	STATE_CONNECTED_READ,
	STATE_CONNECTED_WRITE,
	STATE_CONNECTED_CHANGE_NOTI_INDI,
	STATE_DISCONNECTING
}state_t;


// ******** local function prototypes ********
static cxa_siLabsBgApi_btle_connection_cachedServiceEntry_t* getCachedServiceByUuid(cxa_siLabsBgApi_btle_connection_t *const connIn, cxa_btle_uuid_t *const serviceUuidIn);
static cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* getCachedCharacteristicByUuid(cxa_siLabsBgApi_btle_connection_t *const connIn, cxa_btle_uuid_t *const charUuidIn);
static cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* getCachedCharacteristicByHandle(cxa_siLabsBgApi_btle_connection_t *const connIn, uint16_t handleIn);

static void handleProcedureComplete(cxa_siLabsBgApi_btle_connection_t *const connIn, bool wasSuccessfulIn);

static void stateCb_unused_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_connectingTimeout_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_connIdle_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_connResolveService_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_connResolveChar_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_connRead_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_connWrite_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_connChangeNotiIndi_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_connProcTimeout_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_disconnecting_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_siLabsBgApi_btle_connection_init(cxa_siLabsBgApi_btle_connection_t *const connIn, cxa_btle_client_t *const parentClientIn, int threadIdIn)
{
	cxa_assert(connIn);
	cxa_assert(parentClientIn);

	// save our references and setup our internal state
	connIn->parentClient = parentClientIn;
	cxa_array_initStd(&connIn->cachedServices, connIn->cachedServices_raw);
	cxa_array_initStd(&connIn->cachedCharacteristics, connIn->cachedCharacteristics_raw);
	cxa_logger_init(&connIn->logger, "siLabsBgApiConn");
	connIn->targetProcType = CXA_SILABSBGAPI_PROCTYPE_NONE;
	cxa_fixedByteBuffer_initStd(&connIn->fbb_write, connIn->fbb_write_raw);
	cxa_fixedByteBuffer_initStd(&connIn->fbb_read, connIn->fbb_read_raw);

	// setup our stateMachine
	cxa_stateMachine_init(&connIn->stateMachine, "siLabsBgApiConn", threadIdIn);
	cxa_stateMachine_addState(&connIn->stateMachine, STATE_UNUSED, "unused", stateCb_unused_enter, NULL, NULL, (void*)connIn);
	cxa_stateMachine_addState_timed(&connIn->stateMachine, STATE_CONNECTING, "connecting", STATE_CONNECTING_TIMEOUT, CONNECT_TIMEOUT_MS, stateCb_connecting_enter, NULL, NULL, (void*)connIn);
	cxa_stateMachine_addState(&connIn->stateMachine, STATE_CONNECTING_TIMEOUT, "connTimeout", stateCb_connectingTimeout_enter, NULL, NULL, (void*)connIn);
	cxa_stateMachine_addState(&connIn->stateMachine, STATE_CONNECTED_IDLE, "connIdle", stateCb_connIdle_enter, NULL, NULL, (void*)connIn);
	cxa_stateMachine_addState_timed(&connIn->stateMachine, STATE_CONNECTED_RESOLVE_SERVICE, "connResService", STATE_CONNECTED_PROCEDURE_TIMEOUT, PROCEDURE_TIMEOUT_MS, stateCb_connResolveService_enter, NULL, NULL, (void*)connIn);
	cxa_stateMachine_addState_timed(&connIn->stateMachine, STATE_CONNECTED_RESOLVE_CHAR, "connResChar", STATE_CONNECTED_PROCEDURE_TIMEOUT, PROCEDURE_TIMEOUT_MS, stateCb_connResolveChar_enter, NULL, NULL, (void*)connIn);
	cxa_stateMachine_addState(&connIn->stateMachine, STATE_CONNECTED_READ, "read", stateCb_connRead_enter, NULL, NULL, (void*)connIn);
	cxa_stateMachine_addState(&connIn->stateMachine, STATE_CONNECTED_WRITE, "write", stateCb_connWrite_enter, NULL, NULL, (void*)connIn);
	cxa_stateMachine_addState(&connIn->stateMachine, STATE_CONNECTED_CHANGE_NOTI_INDI, "changeNotiIndi", stateCb_connChangeNotiIndi_enter, NULL, NULL, (void*)connIn);
	cxa_stateMachine_addState(&connIn->stateMachine, STATE_CONNECTED_PROCEDURE_TIMEOUT, "procTimeout", stateCb_connProcTimeout_enter, NULL, NULL, (void*)connIn);
	cxa_stateMachine_addState_timed(&connIn->stateMachine, STATE_DISCONNECTING, "disconnecting", STATE_UNUSED, DISCONNECT_TIMEOUT_MS, stateCb_disconnecting_enter, NULL, NULL, (void*)connIn);

	cxa_stateMachine_setInitialState(&connIn->stateMachine, STATE_UNUSED);
}


bool cxa_siLabsBgApi_btle_connection_isUsed(cxa_siLabsBgApi_btle_connection_t *const connIn)
{
	cxa_assert(connIn);

	return (cxa_stateMachine_getCurrentState(&connIn->stateMachine) != STATE_UNUSED);
}


void cxa_siLabsBgApi_btle_connection_startConnection(cxa_siLabsBgApi_btle_connection_t *const connIn, cxa_eui48_t *const targetAddrIn, bool isRandomAddrIn)
{
	cxa_assert(connIn);
	cxa_assert(targetAddrIn);
	cxa_assert(cxa_stateMachine_getCurrentState(&connIn->stateMachine) == STATE_UNUSED);

	// save our info
	cxa_eui48_initFromEui48(&connIn->targetAddress, targetAddrIn);
	connIn->isRandomAddress = isRandomAddrIn;
	connIn->connHandle = 0;

	cxa_array_clear(&connIn->cachedServices);
	cxa_array_clear(&connIn->cachedCharacteristics);

	cxa_stateMachine_transition(&connIn->stateMachine, STATE_CONNECTING);
}


void cxa_siLabsBgApi_btle_connection_stopConnection(cxa_siLabsBgApi_btle_connection_t *const connIn)
{
	cxa_assert(connIn);
	if( cxa_stateMachine_getCurrentState(&connIn->stateMachine) == STATE_UNUSED )
	{
		// if we're already closed, let them know
		cxa_btle_client_notify_connectionClose(connIn->parentClient, &connIn->targetAddress);
		return;
	}

	connIn->disconnectReason = CXA_BTLE_CLIENT_DISCONNECT_REASON_USER_REQUESTED;
	cxa_stateMachine_transition(&connIn->stateMachine, STATE_DISCONNECTING);
}


void cxa_siLabsBgApi_btle_connection_readFromCharacteristic(cxa_siLabsBgApi_btle_connection_t *const connIn,
															const char *const serviceUuidIn,
															const char *const characteristicUuidIn)
{
	cxa_assert(connIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(characteristicUuidIn);

	// make sure we're in the right state
	if( (cxa_stateMachine_getCurrentState(&connIn->stateMachine) != STATE_CONNECTED_IDLE) ||
		(connIn->targetProcType != CXA_SILABSBGAPI_PROCTYPE_NONE) )
	{
		cxa_logger_warn(&connIn->logger, "incorrect state - rfc");
		cxa_btle_client_notify_readComplete(connIn->parentClient, &connIn->targetAddress, serviceUuidIn, characteristicUuidIn, false, NULL);
		return;
	}

	// save our target service and characteristic
	if( !cxa_btle_uuid_initFromString(&connIn->targetServiceUuid, serviceUuidIn) )
	{
		cxa_logger_warn(&connIn->logger, "invalid service UUID");
		cxa_btle_client_notify_readComplete(connIn->parentClient, &connIn->targetAddress, serviceUuidIn, characteristicUuidIn, false, NULL);
		return;
	}
	if( !cxa_btle_uuid_initFromString(&connIn->targetCharacteristicUuid, characteristicUuidIn) )
	{
		cxa_logger_warn(&connIn->logger, "invalid characteristic UUID");
		cxa_btle_client_notify_readComplete(connIn->parentClient, &connIn->targetAddress, serviceUuidIn, characteristicUuidIn, false, NULL);
		return;
	}

	// see if we have a cached entry for this characteristic
	cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* cachedCharEntry = getCachedCharacteristicByUuid(connIn, &connIn->targetCharacteristicUuid);
	if( cachedCharEntry == NULL )
	{
		// we need to discover this characteristic...see if we have this service cached
		cxa_siLabsBgApi_btle_connection_cachedServiceEntry_t* cachedServiceEntry = getCachedServiceByUuid(connIn, &connIn->targetServiceUuid);
		if( cachedServiceEntry == NULL )
		{
			// we need to discover this service first...
			connIn->targetProcType = CXA_SILABSBGAPI_PROCTYPE_READ;
			cxa_stateMachine_transition(&connIn->stateMachine, STATE_CONNECTED_RESOLVE_SERVICE);
		}
		else
		{
			// we already know about this service...skip to characteristic discovery
			connIn->targetProcType = CXA_SILABSBGAPI_PROCTYPE_READ;
			cxa_stateMachine_transition(&connIn->stateMachine, STATE_CONNECTED_RESOLVE_CHAR);
		}
	}
	else
	{
		// we already know about this characteristic...perform the read
		connIn->targetProcType = CXA_SILABSBGAPI_PROCTYPE_READ;
		cxa_stateMachine_transition(&connIn->stateMachine, STATE_CONNECTED_READ);
	}
}


void cxa_siLabsBgApi_btle_connection_writeToCharacteristic(cxa_siLabsBgApi_btle_connection_t *const connIn,
									  	  	  	  	  	   const char *const serviceUuidIn,
														   const char *const characteristicUuidIn,
														   cxa_fixedByteBuffer_t *const dataIn)
{
	cxa_assert(connIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(characteristicUuidIn);

	// make sure we're in the right state
	if( (cxa_stateMachine_getCurrentState(&connIn->stateMachine) != STATE_CONNECTED_IDLE) ||
		(connIn->targetProcType != CXA_SILABSBGAPI_PROCTYPE_NONE) )
	{
		cxa_logger_warn(&connIn->logger, "incorrect state - wtc");
		cxa_btle_client_notify_writeComplete(connIn->parentClient, &connIn->targetAddress, serviceUuidIn, characteristicUuidIn, false);
		return;
	}

	// save our target service and characteristic
	if( !cxa_btle_uuid_initFromString(&connIn->targetServiceUuid, serviceUuidIn) )
	{
		cxa_logger_warn(&connIn->logger, "invalid service UUID");
		cxa_btle_client_notify_writeComplete(connIn->parentClient, &connIn->targetAddress, serviceUuidIn, characteristicUuidIn, false);
		return;
	}
	if( !cxa_btle_uuid_initFromString(&connIn->targetCharacteristicUuid, characteristicUuidIn) )
	{
		cxa_logger_warn(&connIn->logger, "invalid characteristic UUID");
		cxa_btle_client_notify_writeComplete(connIn->parentClient, &connIn->targetAddress, serviceUuidIn, characteristicUuidIn, false);
		return;
	}

	// copy our data over
	cxa_fixedByteBuffer_clear(&connIn->fbb_write);
	cxa_fixedByteBuffer_append_fbb(&connIn->fbb_write, dataIn);

	// see if we have a cached entry for this characteristic
	cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* cachedCharEntry = getCachedCharacteristicByUuid(connIn, &connIn->targetCharacteristicUuid);
	if( cachedCharEntry == NULL )
	{
		// we need to discover this characteristic...see if we have this service cached
		cxa_siLabsBgApi_btle_connection_cachedServiceEntry_t* cachedServiceEntry = getCachedServiceByUuid(connIn, &connIn->targetServiceUuid);
		if( cachedServiceEntry == NULL )
		{
			// we need to discover this service first...
			connIn->targetProcType = CXA_SILABSBGAPI_PROCTYPE_WRITE;
			cxa_stateMachine_transition(&connIn->stateMachine, STATE_CONNECTED_RESOLVE_SERVICE);
		}
		else
		{
			// we already know about this service...skip to characteristic discovery
			connIn->targetProcType = CXA_SILABSBGAPI_PROCTYPE_WRITE;
			cxa_stateMachine_transition(&connIn->stateMachine, STATE_CONNECTED_RESOLVE_CHAR);
		}
	}
	else
	{
		// we already know about this characteristic...perform the write
		connIn->targetProcType = CXA_SILABSBGAPI_PROCTYPE_WRITE;
		cxa_stateMachine_transition(&connIn->stateMachine, STATE_CONNECTED_WRITE);
	}
}


void cxa_siLabsBgApi_btle_connection_changeNotifications(cxa_siLabsBgApi_btle_connection_t *const connIn,
														 const char *const serviceUuidIn,
														 const char *const characteristicUuidIn,
														 bool enableNotificationsIn)
{
	cxa_assert(connIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(characteristicUuidIn);

	state_t currState = cxa_stateMachine_getCurrentState(&connIn->stateMachine);

	// if we're already disconnected AND unsubscribing, pretend we were successful
	if( (currState == STATE_UNUSED) &&
		!enableNotificationsIn )
	{
		cxa_btle_client_notify_notiIndiSubscriptionChanged(connIn->parentClient, &connIn->targetAddress, serviceUuidIn, characteristicUuidIn, true, enableNotificationsIn);
		return;
	}

	// make sure we're in the right state
	if( (currState != STATE_CONNECTED_IDLE) ||
		(connIn->targetProcType != CXA_SILABSBGAPI_PROCTYPE_NONE) )
	{
		cxa_logger_warn(&connIn->logger, "incorrect state - cn");
		cxa_btle_client_notify_notiIndiSubscriptionChanged(connIn->parentClient, &connIn->targetAddress, serviceUuidIn, characteristicUuidIn, false, false);
		return;
	}

	// save our target service and characteristic
	if( !cxa_btle_uuid_initFromString(&connIn->targetServiceUuid, serviceUuidIn) )
	{
		cxa_logger_warn(&connIn->logger, "invalid service UUID");
		cxa_btle_client_notify_notiIndiSubscriptionChanged(connIn->parentClient, &connIn->targetAddress, serviceUuidIn, characteristicUuidIn, false, false);
		return;
	}
	if( !cxa_btle_uuid_initFromString(&connIn->targetCharacteristicUuid, characteristicUuidIn) )
	{
		cxa_logger_warn(&connIn->logger, "invalid characteristic UUID");
		cxa_btle_client_notify_notiIndiSubscriptionChanged(connIn->parentClient, &connIn->targetAddress, serviceUuidIn, characteristicUuidIn, false, false);
		return;
	}

	// save our intent
	connIn->procEnableNotifications = enableNotificationsIn;

	// see if we have a cached entry for this characteristic
	cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* cachedCharEntry = getCachedCharacteristicByUuid(connIn, &connIn->targetCharacteristicUuid);
	if( cachedCharEntry == NULL )
	{
		// we need to discover this characteristic...see if we have this service cached
		cxa_siLabsBgApi_btle_connection_cachedServiceEntry_t* cachedServiceEntry = getCachedServiceByUuid(connIn, &connIn->targetServiceUuid);
		if( cachedServiceEntry == NULL )
		{
			// we need to discover this service first...
			connIn->targetProcType = CXA_SILABSBGAPI_PROCTYPE_NOTI_INDI_CHANGE;
			cxa_stateMachine_transition(&connIn->stateMachine, STATE_CONNECTED_RESOLVE_SERVICE);
		}
		else
		{
			// we already know about this service...skip to characteristic discovery
			connIn->targetProcType = CXA_SILABSBGAPI_PROCTYPE_NOTI_INDI_CHANGE;
			cxa_stateMachine_transition(&connIn->stateMachine, STATE_CONNECTED_RESOLVE_CHAR);
		}
	}
	else
	{
		// we already know about this characteristic...perform the subscription change
		connIn->targetProcType = CXA_SILABSBGAPI_PROCTYPE_NOTI_INDI_CHANGE;
		cxa_stateMachine_transition(&connIn->stateMachine, STATE_CONNECTED_CHANGE_NOTI_INDI);
	}
}


void cxa_siLabsBgApi_btle_connection_handleEvent_opened(cxa_siLabsBgApi_btle_connection_t *const connIn, uint8_t newConnHandleIn)
{
	cxa_assert(connIn);

	// save our connection information
	connIn->connHandle = newConnHandleIn;

	cxa_stateMachine_transitionNow(&connIn->stateMachine, STATE_CONNECTED_IDLE);
	cxa_btle_client_notify_connectionStarted(connIn->parentClient, &connIn->targetAddress);
}


void cxa_siLabsBgApi_btle_connection_handleEvent_closed(cxa_siLabsBgApi_btle_connection_t *const connIn, uint16_t reasonCodeIn)
{
	cxa_assert(connIn);

	// ascertain our reason based upon our state
	switch( cxa_stateMachine_getCurrentState(&connIn->stateMachine) )
	{
		case STATE_CONNECTING_TIMEOUT:
		case STATE_DISCONNECTING:
			// we've already set the disconnect reason
			break;

		default:
			connIn->disconnectReason = CXA_BTLE_CLIENT_DISCONNECT_REASON_STACK;
			break;
	}

	// start the transition
	cxa_eui48_string_t targetAddr_str;
	cxa_eui48_toString(&connIn->targetAddress, &targetAddr_str);

	cxa_logger_warn(&connIn->logger, "connection to '%s' closed", targetAddr_str.str);
	cxa_stateMachine_transitionNow(&connIn->stateMachine, STATE_UNUSED);
}


void cxa_siLabsBgApi_btle_connection_handleEvent_serviceResolved(cxa_siLabsBgApi_btle_connection_t *const connIn, cxa_btle_uuid_t *const uuid, uint32_t handleIn)
{
	cxa_assert(connIn);

	cxa_siLabsBgApi_btle_connection_cachedServiceEntry_t* newEntry = cxa_array_append_empty(&connIn->cachedServices);
	if( newEntry == NULL )
	{
		cxa_logger_warn(&connIn->logger, "too many cached services");
		return;
	}

	newEntry->handle = handleIn;
	cxa_btle_uuid_initFromUuid(&newEntry->uuid, uuid, false);
}


void cxa_siLabsBgApi_btle_connection_handleEvent_characteristicResolved(cxa_siLabsBgApi_btle_connection_t *const connIn, cxa_btle_uuid_t *const uuid, uint16_t handleIn)
{
	cxa_assert(connIn);

	cxa_siLabsBgApi_btle_connection_cachedServiceEntry_t* targetServiceEntry = getCachedServiceByUuid(connIn, &connIn->targetServiceUuid);
	if( targetServiceEntry == NULL )
	{
		cxa_logger_warn(&connIn->logger, "couldn't resolve service for characteristic");
		return;
	}

	cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* newEntry = cxa_array_append_empty(&connIn->cachedCharacteristics);
	if( newEntry == NULL )
	{
		cxa_logger_warn(&connIn->logger, "too many cached characteristics");
		return;
	}

	newEntry->handle = handleIn;
	cxa_btle_uuid_initFromUuid(&newEntry->uuid, uuid, false);
	newEntry->service = targetServiceEntry;
}


void cxa_siLabsBgApi_btle_connection_handleEvent_characteristicValueUpdated(cxa_siLabsBgApi_btle_connection_t *const connIn, uint16_t handleIn, enum gatt_att_opcode opcodeIn, uint8_t *const dataIn, size_t dataLen_bytesIn)
{
	cxa_assert(connIn);

	// make sure we're in the right state
	state_t currState = cxa_stateMachine_getCurrentState(&connIn->stateMachine);
	if( (currState == STATE_UNUSED) ||
		(currState == STATE_CONNECTING) )
	{
		return;
	}
	// if we made it here, we're in the correct state

	switch( opcodeIn )
	{
		case gatt_read_request:
		case gatt_read_response:
		{
			// make sure the handle matches the target characteristic
			cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* updatedChar = getCachedCharacteristicByHandle(connIn, handleIn);
			if( updatedChar != NULL )
			{
				cxa_fixedByteBuffer_clear(&connIn->fbb_read);
				if( !cxa_fixedByteBuffer_append(&connIn->fbb_read, dataIn, dataLen_bytesIn) )
				{
					cxa_logger_warn(&connIn->logger, "too many read bytes, increase `CXA_SILABSBGAPI_BTLE_CONNECTION_BUFFER_SIZE_BYTES`");
					return;
				}
			}
			break;
		}

		case gatt_handle_value_notification:
		case gatt_handle_value_indication:
		{
			cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* updatedChar = getCachedCharacteristicByHandle(connIn, handleIn);
			if( updatedChar != NULL )
			{
				cxa_btle_uuid_string_t str_serviceUuid;
				cxa_btle_uuid_string_t str_charUuid;
				cxa_btle_uuid_toString(&updatedChar->service->uuid, &str_serviceUuid);
				cxa_btle_uuid_toString(&updatedChar->uuid, &str_charUuid);
				cxa_fixedByteBuffer_t tmpBuffer;
				cxa_fixedByteBuffer_init_inPlace(&tmpBuffer, dataLen_bytesIn, dataIn, dataLen_bytesIn);

				cxa_btle_client_notify_notiIndiRx(connIn->parentClient, &connIn->targetAddress, str_serviceUuid.str, str_charUuid.str, &tmpBuffer);
			}
			break;
		}

		default:
			// do nothing
			break;
	}
}


void cxa_siLabsBgApi_btle_connection_handleEvent_procedureComplete(cxa_siLabsBgApi_btle_connection_t *const connIn, uint16_t resultCodeIn)
{
	cxa_assert(connIn);

	if( resultCodeIn != 0 )
	{
		handleProcedureComplete(connIn, false);
		return;
	}
	// if we made it here, we were successful

	// depends on our current state
	switch( cxa_stateMachine_getCurrentState(&connIn->stateMachine) )
	{
		case STATE_CONNECTED_RESOLVE_SERVICE:
			cxa_stateMachine_transition(&connIn->stateMachine, STATE_CONNECTED_RESOLVE_CHAR);
			break;

		case STATE_CONNECTED_RESOLVE_CHAR:
			switch( connIn->targetProcType )
			{
				case CXA_SILABSBGAPI_PROCTYPE_READ:
					cxa_stateMachine_transition(&connIn->stateMachine, STATE_CONNECTED_READ);
					break;

				case CXA_SILABSBGAPI_PROCTYPE_WRITE:
					cxa_stateMachine_transition(&connIn->stateMachine, STATE_CONNECTED_WRITE);
					break;

				case CXA_SILABSBGAPI_PROCTYPE_NOTI_INDI_CHANGE:
					cxa_stateMachine_transition(&connIn->stateMachine, STATE_CONNECTED_CHANGE_NOTI_INDI);
					break;

				default:
					cxa_logger_warn(&connIn->logger, "unspecified target procedure, aborting");
					cxa_stateMachine_transitionNow(&connIn->stateMachine, STATE_CONNECTED_IDLE);
					break;
			}
			break;

		case STATE_CONNECTED_READ:
			cxa_logger_debug(&connIn->logger, "read complete with code: %d", resultCodeIn);
			handleProcedureComplete(connIn, true);
			break;

		case STATE_CONNECTED_WRITE:
			cxa_logger_debug(&connIn->logger, "write complete with code: %d", resultCodeIn);
			handleProcedureComplete(connIn, true);
			break;

		case STATE_CONNECTED_CHANGE_NOTI_INDI:
			cxa_logger_debug(&connIn->logger, "noti/indi change complete with code: %d", resultCodeIn);
			handleProcedureComplete(connIn, true);
			break;
	}
}


// ******** local function implementations ********
static cxa_siLabsBgApi_btle_connection_cachedServiceEntry_t* getCachedServiceByUuid(cxa_siLabsBgApi_btle_connection_t *const connIn, cxa_btle_uuid_t *const serviceUuidIn)
{
	cxa_assert(connIn);
	cxa_assert(serviceUuidIn);

	cxa_siLabsBgApi_btle_connection_cachedServiceEntry_t* retVal = NULL;

	cxa_array_iterate(&connIn->cachedServices, currService, cxa_siLabsBgApi_btle_connection_cachedServiceEntry_t)
	{
		if( currService == NULL ) continue;

		if( cxa_btle_uuid_isEqual(&currService->uuid, serviceUuidIn) )
		{
			retVal = currService;
			break;
		}
	}

	return retVal;
}


static cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* getCachedCharacteristicByUuid(cxa_siLabsBgApi_btle_connection_t *const connIn, cxa_btle_uuid_t *const charUuidIn)
{
	cxa_assert(connIn);
	cxa_assert(charUuidIn);

	cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* retVal = NULL;

	cxa_array_iterate(&connIn->cachedCharacteristics, currChar, cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t)
	{
		if( currChar == NULL ) continue;

		if( cxa_btle_uuid_isEqual(&currChar->uuid, charUuidIn) )
		{
			retVal = currChar;
			break;
		}
	}

	return retVal;
}


static cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* getCachedCharacteristicByHandle(cxa_siLabsBgApi_btle_connection_t *const connIn, uint16_t handleIn)
{
	cxa_assert(connIn);

	cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* retVal = NULL;

	cxa_array_iterate(&connIn->cachedCharacteristics, currChar, cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t)
	{
		if( currChar == NULL ) continue;

		if( currChar->handle == handleIn )
		{
			retVal = currChar;
			break;
		}
	}

	return retVal;
}


static void handleProcedureComplete(cxa_siLabsBgApi_btle_connection_t *const connIn, bool wasSuccessfulIn)
{
	cxa_assert(connIn);

	cxa_btle_uuid_string_t str_serviceUuid;
	cxa_btle_uuid_string_t str_charUuid;
	cxa_btle_uuid_toString(&connIn->targetServiceUuid, &str_serviceUuid);
	cxa_btle_uuid_toString(&connIn->targetCharacteristicUuid, &str_charUuid);

	// record our previous procedure type
	cxa_siLabsBgApi_btle_connection_procType_t prevProcType = connIn->targetProcType;

	// transition immediately first (so our listener can take action)
	cxa_stateMachine_transitionNow(&connIn->stateMachine, STATE_CONNECTED_IDLE);

	// now that we're idle, call our listener
	switch( prevProcType )
	{
		case CXA_SILABSBGAPI_PROCTYPE_READ:
			cxa_btle_client_notify_readComplete(connIn->parentClient, &connIn->targetAddress, str_serviceUuid.str, str_charUuid.str, wasSuccessfulIn, &connIn->fbb_read);
			break;

		case CXA_SILABSBGAPI_PROCTYPE_WRITE:
			cxa_btle_client_notify_writeComplete(connIn->parentClient, &connIn->targetAddress, str_serviceUuid.str, str_charUuid.str, wasSuccessfulIn);
			break;

		case CXA_SILABSBGAPI_PROCTYPE_NOTI_INDI_CHANGE:
			cxa_btle_client_notify_notiIndiSubscriptionChanged(connIn->parentClient, &connIn->targetAddress, str_serviceUuid.str, str_charUuid.str, wasSuccessfulIn, connIn->procEnableNotifications);
			break;

		default:
			break;
	}
}


static void stateCb_unused_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_connection_t *const connIn = (cxa_siLabsBgApi_btle_connection_t *const)userVarIn;
	cxa_assert(connIn);

	if( prevStateIdIn != CXA_STATE_MACHINE_STATE_UNKNOWN )
	{
		switch( connIn->disconnectReason )
		{
			case CXA_BTLE_CLIENT_DISCONNECT_REASON_USER_REQUESTED:
				cxa_btle_client_notify_connectionClose_expected(connIn->parentClient, &connIn->targetAddress);
				break;

			default:
				cxa_btle_client_notify_connectionClose_unexpected(connIn->parentClient, &connIn->targetAddress, connIn->disconnectReason);
				break;
		}

	}
}


static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_connection_t *const connIn = (cxa_siLabsBgApi_btle_connection_t *const)userVarIn;
	cxa_assert(connIn);

	bd_addr targetAddr;
	memcpy(targetAddr.addr, connIn->targetAddress.bytes, sizeof(targetAddr.addr));

	cxa_eui48_string_t targetAddr_str;
	cxa_eui48_toString(&connIn->targetAddress, &targetAddr_str);

	cxa_logger_debug(&connIn->logger, "connecting to '%s'", targetAddr_str.str);
	struct gecko_msg_le_gap_connect_rsp_t* rsp = gecko_cmd_le_gap_connect(targetAddr, le_gap_address_type_public, le_gap_phy_1m);
	if( rsp->result != 0 )
	{
		cxa_logger_warn(&connIn->logger, "connection failed: %d", rsp->result);
		connIn->disconnectReason = CXA_BTLE_CLIENT_DISCONNECT_REASON_STACK;
		cxa_stateMachine_transitionNow(&connIn->stateMachine, STATE_UNUSED);
		return;
	}
}


static void stateCb_connectingTimeout_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_connection_t *const connIn = (cxa_siLabsBgApi_btle_connection_t *const)userVarIn;
	cxa_assert(connIn);

	cxa_logger_warn(&connIn->logger, "connection timeout");
	connIn->disconnectReason = CXA_BTLE_CLIENT_DISCONNECT_REASON_CONNECTION_TIMEOUT;
	cxa_stateMachine_transition(&connIn->stateMachine, STATE_DISCONNECTING);
}


static void stateCb_connIdle_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_connection_t *const connIn = (cxa_siLabsBgApi_btle_connection_t *const)userVarIn;
	cxa_assert(connIn);

	cxa_logger_debug(&connIn->logger, "connected and idle");

	connIn->targetProcType = CXA_SILABSBGAPI_PROCTYPE_NONE;
}


static void stateCb_connResolveService_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_connection_t *const connIn = (cxa_siLabsBgApi_btle_connection_t *const)userVarIn;
	cxa_assert(connIn);

	cxa_btle_uuid_string_t str_uuid;
	cxa_btle_uuid_toString(&connIn->targetServiceUuid, &str_uuid);
	cxa_logger_debug(&connIn->logger, "resolving service '%s'", str_uuid.str);

	// resolve our service
	cxa_btle_uuid_t uuid_transposed;
	cxa_btle_uuid_initFromUuid(&uuid_transposed, &connIn->targetServiceUuid, true);
	uint8_t uuidLen = (uuid_transposed.type == CXA_BTLE_UUID_TYPE_16BIT) ? (16/8) : (128/8);
	uint8_t* uuidBytes = (uuid_transposed.type == CXA_BTLE_UUID_TYPE_16BIT) ? (uint8_t*)&uuid_transposed.as16Bit : uuid_transposed.as128Bit.bytes;

	struct gecko_msg_gatt_discover_primary_services_by_uuid_rsp_t* rsp = gecko_cmd_gatt_discover_primary_services_by_uuid(connIn->connHandle, uuidLen, uuidBytes);
	if( rsp->result != 0 )
	{
		cxa_logger_warn(&connIn->logger, "error discovering service: %d", rsp->result);
		connIn->procSuccessful = false;
		cxa_stateMachine_transition(&connIn->stateMachine, STATE_CONNECTED_IDLE);
		return;
	}
}


static void stateCb_connResolveChar_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_connection_t *const connIn = (cxa_siLabsBgApi_btle_connection_t *const)userVarIn;
	cxa_assert(connIn);

	// get our service (we should have resolved this already)
	cxa_siLabsBgApi_btle_connection_cachedServiceEntry_t* cachedServiceEntry = getCachedServiceByUuid(connIn, &connIn->targetServiceUuid);
	if( cachedServiceEntry == NULL )
	{
		cxa_logger_warn(&connIn->logger, "unknown service");
		handleProcedureComplete(connIn, false);
		return;
	}

	cxa_btle_uuid_string_t str_uuid;
	cxa_btle_uuid_toString(&connIn->targetCharacteristicUuid, &str_uuid);
	cxa_logger_debug(&connIn->logger, "resolving characteristic '%s'", str_uuid.str);

	// resolve our characteristic
	cxa_btle_uuid_t uuid_transposed;
	cxa_btle_uuid_initFromUuid(&uuid_transposed, &connIn->targetCharacteristicUuid, true);
	uint8_t uuidLen = (uuid_transposed.type == CXA_BTLE_UUID_TYPE_16BIT) ? (16/8) : (128/8);
	uint8_t* uuidBytes = (uuid_transposed.type == CXA_BTLE_UUID_TYPE_16BIT) ? (uint8_t*)&uuid_transposed.as16Bit : uuid_transposed.as128Bit.bytes;

	struct gecko_msg_gatt_discover_characteristics_by_uuid_rsp_t* rsp = gecko_cmd_gatt_discover_characteristics_by_uuid(connIn->connHandle, cachedServiceEntry->handle, uuidLen, uuidBytes);
	if( rsp->result != 0 )
	{
		cxa_logger_warn(&connIn->logger, "error discovering characteristic: %d", rsp->result);
		handleProcedureComplete(connIn, false);
		return;
	}
}


static void stateCb_connRead_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_connection_t *const connIn = (cxa_siLabsBgApi_btle_connection_t *const)userVarIn;
	cxa_assert(connIn);

	cxa_btle_uuid_string_t str_serviceUuid;
	cxa_btle_uuid_string_t str_charUuid;
	cxa_btle_uuid_toString(&connIn->targetServiceUuid, &str_serviceUuid);
	cxa_btle_uuid_toString(&connIn->targetCharacteristicUuid, &str_charUuid);

	cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* cachedCharEntry = getCachedCharacteristicByUuid(connIn, &connIn->targetCharacteristicUuid);
	if( cachedCharEntry == NULL )
	{
		cxa_logger_warn(&connIn->logger, "unknown characteristic");
		handleProcedureComplete(connIn, false);
		return;
	}

	struct gecko_msg_gatt_read_characteristic_value_rsp_t* rsp = gecko_cmd_gatt_read_characteristic_value(connIn->connHandle, cachedCharEntry->handle);
	if( rsp->result != 0 )
	{
		cxa_logger_warn(&connIn->logger, "read failed: %d", rsp->result);

		cxa_btle_client_notify_readComplete(connIn->parentClient, &connIn->targetAddress, str_serviceUuid.str, str_charUuid.str, false, NULL);
		return;
	}
	// if we made it here, the read was successful
	cxa_logger_debug(&connIn->logger, "read complete, awaiting response");
}


static void stateCb_connWrite_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_connection_t *const connIn = (cxa_siLabsBgApi_btle_connection_t *const)userVarIn;
	cxa_assert(connIn);

	cxa_btle_uuid_string_t str_serviceUuid;
	cxa_btle_uuid_string_t str_charUuid;
	cxa_btle_uuid_toString(&connIn->targetServiceUuid, &str_serviceUuid);
	cxa_btle_uuid_toString(&connIn->targetCharacteristicUuid, &str_charUuid);

	cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* cachedCharEntry = getCachedCharacteristicByUuid(connIn, &connIn->targetCharacteristicUuid);
	if( cachedCharEntry == NULL )
	{
		cxa_logger_warn(&connIn->logger, "unknown characteristic");
		handleProcedureComplete(connIn, false);
		return;
	}

	cxa_logger_debug_memDump_fbb(&connIn->logger, "writing ", &connIn->fbb_write, NULL);
	struct gecko_msg_gatt_write_characteristic_value_rsp_t* rsp = gecko_cmd_gatt_write_characteristic_value(connIn->connHandle,
																											cachedCharEntry->handle,
																											cxa_fixedByteBuffer_getSize_bytes(&connIn->fbb_write),
																											cxa_fixedByteBuffer_get_pointerToIndex(&connIn->fbb_write,  0));
	if( rsp->result != 0 )
	{
		cxa_logger_warn(&connIn->logger, "write failed: %d", rsp->result);

		cxa_btle_client_notify_writeComplete(connIn->parentClient, &connIn->targetAddress, str_serviceUuid.str, str_charUuid.str, false);
		return;
	}
	// if we made it here, the write was successful
	cxa_logger_debug(&connIn->logger, "write complete, awaiting response");
}


static void stateCb_connChangeNotiIndi_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_connection_t *const connIn = (cxa_siLabsBgApi_btle_connection_t *const)userVarIn;
	cxa_assert(connIn);

	cxa_btle_uuid_string_t str_serviceUuid;
	cxa_btle_uuid_string_t str_charUuid;
	cxa_btle_uuid_toString(&connIn->targetServiceUuid, &str_serviceUuid);
	cxa_btle_uuid_toString(&connIn->targetCharacteristicUuid, &str_charUuid);

	cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* cachedCharEntry = getCachedCharacteristicByUuid(connIn, &connIn->targetCharacteristicUuid);
	if( cachedCharEntry == NULL )
	{
		cxa_logger_warn(&connIn->logger, "unknown characteristic");
		handleProcedureComplete(connIn, false);
		return;
	}

	struct gecko_msg_gatt_set_characteristic_notification_rsp_t* rsp = gecko_cmd_gatt_set_characteristic_notification(connIn->connHandle, cachedCharEntry->handle, (connIn->procEnableNotifications) ? 0x01 : 0x00);
	if( rsp->result != 0 )
	{
		cxa_logger_warn(&connIn->logger, "write failed: %d", rsp->result);

		cxa_btle_client_notify_notiIndiSubscriptionChanged(connIn->parentClient, &connIn->targetAddress, str_serviceUuid.str, str_charUuid.str, false, false);
		return;
	}
	// if we made it here, the write was successful
	cxa_logger_debug(&connIn->logger, "subscription change complete, awaiting response");
}


static void stateCb_connProcTimeout_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_connection_t *const connIn = (cxa_siLabsBgApi_btle_connection_t *const)userVarIn;
	cxa_assert(connIn);

	cxa_logger_warn(&connIn->logger, "procedure timeout");
	cxa_siLabsBgApi_btle_connection_stopConnection(connIn);
}


static void stateCb_disconnecting_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_connection_t *const connIn = (cxa_siLabsBgApi_btle_connection_t *const)userVarIn;
	cxa_assert(connIn);

	cxa_eui48_string_t targetAddr_str;
	cxa_eui48_toString(&connIn->targetAddress, &targetAddr_str);

	cxa_logger_warn(&connIn->logger, "closing connection to '%s'", targetAddr_str.str);
	gecko_cmd_le_connection_close(connIn->connHandle);
}
