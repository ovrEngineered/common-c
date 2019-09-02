/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_siLabsBgApi_btle_connection.h"


// ******** includes ********
#include <cxa_assert.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_DEBUG
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
static cxa_siLabsBgApi_btle_connection_cachedServiceEntry_t* getCachedServiceByUuid(cxa_siLabsBgApi_btle_connection_t *const connIn, const char *const serviceUuidStrIn);
static cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* getCachedCharacteristicByUuid(cxa_siLabsBgApi_btle_connection_t *const connIn, const char *const charUuidStrIn);
static cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* getCachedCharacteristicByHandle(cxa_siLabsBgApi_btle_connection_t *const connIn, uint16_t handleIn);

static void handleProcedureComplete(cxa_siLabsBgApi_btle_connection_t *const connIn, bool wasSuccessfulIn);

static void scm_stopConnection(cxa_btle_connection_t *const superIn);
static void scm_readFromCharacteristic(cxa_btle_connection_t *const superIn, const char *const serviceUuidIn, const char *const characteristicUuidIn);
static void scm_writeToCharacteristic(cxa_btle_connection_t *const superIn, const char *const serviceUuidIn, const char *const characteristicUuidIn, cxa_fixedByteBuffer_t *const dataIn);
static void scm_changeNotifications(cxa_btle_connection_t *const superIn, const char *const serviceUuidIn, const char *const characteristicUuidIn, bool enableNotifications);

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
void cxa_siLabsBgApi_btle_connection_init(cxa_siLabsBgApi_btle_connection_t *const connIn, cxa_btle_central_t *const parentClientIn, int threadIdIn)
{
	cxa_assert(connIn);
	cxa_assert(parentClientIn);

	// initialize our superclass
	cxa_btle_connection_init(&connIn->super, parentClientIn, scm_stopConnection, scm_readFromCharacteristic, scm_writeToCharacteristic, scm_changeNotifications);

	// save our references and setup our internal state
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
	cxa_btle_connection_setTargetAddress(&connIn->super, targetAddrIn);
	connIn->isRandomAddress = isRandomAddrIn;
	connIn->connHandle = 0;

	cxa_array_clear(&connIn->cachedServices);
	cxa_array_clear(&connIn->cachedCharacteristics);

	cxa_stateMachine_transition(&connIn->stateMachine, STATE_CONNECTING);

}


void cxa_siLabsBgApi_btle_connection_handleEvent_opened(cxa_siLabsBgApi_btle_connection_t *const connIn)
{
	cxa_assert(connIn);

	cxa_stateMachine_transitionNow(&connIn->stateMachine, STATE_CONNECTED_IDLE);
	cxa_btle_central_notify_connectionStarted((cxa_btle_central_t*)connIn->super.btlec, true, &connIn->super);
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
			connIn->disconnectReason = CXA_BTLE_CONNECTION_DISCONNECT_REASON_STACK;
			break;
	}

	// start the transition
	cxa_eui48_string_t targetAddr_str;
	cxa_eui48_toString(cxa_btle_connection_getTargetMacAddress(&connIn->super), &targetAddr_str);

	cxa_logger_debug(&connIn->logger, "connection to '%s' closed", targetAddr_str.str);
	cxa_btle_connection_notify_connectionClose(&connIn->super, connIn->disconnectReason);

	cxa_stateMachine_transitionNow(&connIn->stateMachine, STATE_UNUSED);
}


void cxa_siLabsBgApi_btle_connection_handleEvent_serviceResolved(cxa_siLabsBgApi_btle_connection_t *const connIn, cxa_btle_uuid_t *const uuidIn, uint32_t handleIn)
{
	cxa_assert(connIn);

	// make sure the resolved uuid matches our target uuid
	if( !cxa_btle_uuid_isEqualToString(uuidIn, connIn->targetServiceUuid_str) )
	{
		cxa_logger_warn(&connIn->logger, "resolution failed");
		return;
	}

	cxa_siLabsBgApi_btle_connection_cachedServiceEntry_t* newEntry = cxa_array_append_empty(&connIn->cachedServices);
	if( newEntry == NULL )
	{
		cxa_logger_warn(&connIn->logger, "too many cached services");
		return;
	}

	newEntry->handle = handleIn;
	newEntry->uuid_str = connIn->targetServiceUuid_str;
}


void cxa_siLabsBgApi_btle_connection_handleEvent_characteristicResolved(cxa_siLabsBgApi_btle_connection_t *const connIn, cxa_btle_uuid_t *const uuidIn, uint16_t handleIn)
{
	cxa_assert(connIn);

	// make sure the resolved uuid matches our target uuid
	if( !cxa_btle_uuid_isEqualToString(uuidIn, connIn->targetCharacteristicUuid_str) )
	{
		cxa_logger_warn(&connIn->logger, "resolution failed");
		return;
	}

	cxa_siLabsBgApi_btle_connection_cachedServiceEntry_t* targetServiceEntry = getCachedServiceByUuid(connIn, connIn->targetServiceUuid_str);
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
	newEntry->uuid_str = connIn->targetCharacteristicUuid_str;
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
				cxa_fixedByteBuffer_t tmpBuffer;
				cxa_fixedByteBuffer_init_inPlace(&tmpBuffer, dataLen_bytesIn, dataIn, dataLen_bytesIn);

				cxa_btle_connection_notify_notiIndiRx(&connIn->super, updatedChar->service->uuid_str, updatedChar->uuid_str, &tmpBuffer);
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
static cxa_siLabsBgApi_btle_connection_cachedServiceEntry_t* getCachedServiceByUuid(cxa_siLabsBgApi_btle_connection_t *const connIn, const char *const serviceUuidStrIn)
{
	cxa_assert(connIn);
	cxa_assert(serviceUuidStrIn);

	cxa_btle_uuid_t targetServiceUuid;
	if( !cxa_btle_uuid_initFromString(&targetServiceUuid, serviceUuidStrIn) ) return NULL;

	cxa_siLabsBgApi_btle_connection_cachedServiceEntry_t* retVal = NULL;

	cxa_btle_uuid_t currServiceUuid;
	cxa_array_iterate(&connIn->cachedServices, currService, cxa_siLabsBgApi_btle_connection_cachedServiceEntry_t)
	{
		if( currService == NULL ) continue;

		if( !cxa_btle_uuid_initFromString(&currServiceUuid, currService->uuid_str) ) continue;

		if( cxa_btle_uuid_isEqual(&targetServiceUuid, &currServiceUuid) )
		{
			retVal = currService;
			break;
		}
	}

	return retVal;
}


static cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* getCachedCharacteristicByUuid(cxa_siLabsBgApi_btle_connection_t *const connIn, const char *const charUuidStrIn)
{
	cxa_assert(connIn);
	cxa_assert(charUuidStrIn);

	cxa_btle_uuid_t targetCharUuid;
	if( !cxa_btle_uuid_initFromString(&targetCharUuid, charUuidStrIn) ) return NULL;

	cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* retVal = NULL;

	cxa_btle_uuid_t currCharUuid;
	cxa_array_iterate(&connIn->cachedCharacteristics, currChar, cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t)
	{
		if( currChar == NULL ) continue;

		if( !cxa_btle_uuid_initFromString(&currCharUuid, currChar->uuid_str) ) continue;

		if( cxa_btle_uuid_isEqual(&targetCharUuid, &currCharUuid) )
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

	// record our previous procedure type
	cxa_siLabsBgApi_btle_connection_procType_t prevProcType = connIn->targetProcType;
	connIn->targetProcType = CXA_SILABSBGAPI_PROCTYPE_NONE;

	// transition immediately first (so our listener can take action)
	cxa_stateMachine_transitionNow(&connIn->stateMachine, STATE_CONNECTED_IDLE);

	// now that we're idle, call our listener
	switch( prevProcType )
	{
		case CXA_SILABSBGAPI_PROCTYPE_READ:
			cxa_btle_connection_notify_readComplete(&connIn->super, connIn->targetServiceUuid_str, connIn->targetCharacteristicUuid_str, wasSuccessfulIn, &connIn->fbb_read);
			break;

		case CXA_SILABSBGAPI_PROCTYPE_WRITE:
			cxa_btle_connection_notify_writeComplete(&connIn->super, connIn->targetServiceUuid_str, connIn->targetCharacteristicUuid_str, wasSuccessfulIn);
			break;

		case CXA_SILABSBGAPI_PROCTYPE_NOTI_INDI_CHANGE:
			cxa_btle_connection_notify_notiIndiSubscriptionChanged(&connIn->super, connIn->targetServiceUuid_str, connIn->targetCharacteristicUuid_str, wasSuccessfulIn, connIn->procEnableNotifications);
			break;

		default:
			break;
	}
}


static void scm_stopConnection(cxa_btle_connection_t *const superIn)
{
	cxa_siLabsBgApi_btle_connection_t *const connIn = (cxa_siLabsBgApi_btle_connection_t *const)superIn;
	cxa_assert(connIn);

	if( cxa_stateMachine_getCurrentState(&connIn->stateMachine) == STATE_UNUSED )
	{
		// if we're already closed, let them know
		cxa_btle_connection_notify_connectionClose(&connIn->super, CXA_BTLE_CONNECTION_DISCONNECT_REASON_USER_REQUESTED);
		return;
	}

	connIn->disconnectReason = CXA_BTLE_CONNECTION_DISCONNECT_REASON_USER_REQUESTED;
	cxa_stateMachine_transition(&connIn->stateMachine, STATE_DISCONNECTING);
}


static void scm_readFromCharacteristic(cxa_btle_connection_t *const superIn, const char *const serviceUuidIn, const char *const characteristicUuidIn)
{
	cxa_siLabsBgApi_btle_connection_t *const connIn = (cxa_siLabsBgApi_btle_connection_t *const)superIn;
	cxa_assert(connIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(characteristicUuidIn);

	// make sure we're in the right state
	if( (cxa_stateMachine_getCurrentState(&connIn->stateMachine) != STATE_CONNECTED_IDLE) ||
		(connIn->targetProcType != CXA_SILABSBGAPI_PROCTYPE_NONE) )
	{
		cxa_logger_warn(&connIn->logger, "incorrect state - rfc - %d/%d", cxa_stateMachine_getCurrentState(&connIn->stateMachine), connIn->targetProcType);
		cxa_btle_connection_notify_readComplete(&connIn->super, serviceUuidIn, characteristicUuidIn, false, NULL);
		return;
	}

	// save our target service and characteristic
	connIn->targetServiceUuid_str = serviceUuidIn;
	connIn->targetCharacteristicUuid_str = characteristicUuidIn;

	// see if we have a cached entry for this characteristic
	cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* cachedCharEntry = getCachedCharacteristicByUuid(connIn, connIn->targetCharacteristicUuid_str);
	if( cachedCharEntry == NULL )
	{
		// we need to discover this characteristic...see if we have this service cached
		cxa_siLabsBgApi_btle_connection_cachedServiceEntry_t* cachedServiceEntry = getCachedServiceByUuid(connIn, connIn->targetServiceUuid_str);
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


static void scm_writeToCharacteristic(cxa_btle_connection_t *const superIn, const char *const serviceUuidIn, const char *const characteristicUuidIn, cxa_fixedByteBuffer_t *const dataIn)
{
	cxa_siLabsBgApi_btle_connection_t *const connIn = (cxa_siLabsBgApi_btle_connection_t *const)superIn;
	cxa_assert(connIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(characteristicUuidIn);

	// make sure we're in the right state
	if( (cxa_stateMachine_getCurrentState(&connIn->stateMachine) != STATE_CONNECTED_IDLE) ||
		(connIn->targetProcType != CXA_SILABSBGAPI_PROCTYPE_NONE) )
	{
		cxa_logger_warn(&connIn->logger, "incorrect state - wtc - %d/%d", cxa_stateMachine_getCurrentState(&connIn->stateMachine), connIn->targetProcType);
		cxa_btle_connection_notify_writeComplete(&connIn->super, serviceUuidIn, characteristicUuidIn, false);
		return;
	}

	// save our target service and characteristic
	connIn->targetServiceUuid_str = serviceUuidIn;
	connIn->targetCharacteristicUuid_str = characteristicUuidIn;

	// copy our data over
	cxa_fixedByteBuffer_clear(&connIn->fbb_write);
	cxa_fixedByteBuffer_append_fbb(&connIn->fbb_write, dataIn);

	// see if we have a cached entry for this characteristic
	cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* cachedCharEntry = getCachedCharacteristicByUuid(connIn, connIn->targetCharacteristicUuid_str);
	if( cachedCharEntry == NULL )
	{
		// we need to discover this characteristic...see if we have this service cached
		cxa_siLabsBgApi_btle_connection_cachedServiceEntry_t* cachedServiceEntry = getCachedServiceByUuid(connIn, connIn->targetServiceUuid_str);
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


static void scm_changeNotifications(cxa_btle_connection_t *const superIn, const char *const serviceUuidIn, const char *const characteristicUuidIn, bool enableNotificationsIn)
{
	cxa_siLabsBgApi_btle_connection_t *const connIn = (cxa_siLabsBgApi_btle_connection_t *const)superIn;
	cxa_assert(connIn);
	cxa_assert(serviceUuidIn);
	cxa_assert(characteristicUuidIn);

	state_t currState = cxa_stateMachine_getCurrentState(&connIn->stateMachine);

	// if we're already disconnected AND unsubscribing, pretend we were successful
	if( (currState == STATE_UNUSED) &&
		!enableNotificationsIn )
	{
		cxa_btle_connection_notify_notiIndiSubscriptionChanged(&connIn->super, serviceUuidIn, characteristicUuidIn, true, enableNotificationsIn);
		return;
	}

	// make sure we're in the right state
	if( (currState != STATE_CONNECTED_IDLE) ||
		(connIn->targetProcType != CXA_SILABSBGAPI_PROCTYPE_NONE) )
	{
		cxa_logger_warn(&connIn->logger, "incorrect state - cn - %d/%d", currState, connIn->targetProcType);
		cxa_btle_connection_notify_notiIndiSubscriptionChanged(&connIn->super, serviceUuidIn, characteristicUuidIn, false, false);
		return;
	}

	// save our target service and characteristic
	connIn->targetServiceUuid_str = serviceUuidIn;
	connIn->targetCharacteristicUuid_str = characteristicUuidIn;

	// save our intent
	connIn->procEnableNotifications = enableNotificationsIn;

	// see if we have a cached entry for this characteristic
	cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* cachedCharEntry = getCachedCharacteristicByUuid(connIn, connIn->targetCharacteristicUuid_str);
	if( cachedCharEntry == NULL )
	{
		// we need to discover this characteristic...see if we have this service cached
		cxa_siLabsBgApi_btle_connection_cachedServiceEntry_t* cachedServiceEntry = getCachedServiceByUuid(connIn, connIn->targetCharacteristicUuid_str);
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


static void stateCb_unused_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_connection_t *const connIn = (cxa_siLabsBgApi_btle_connection_t *const)userVarIn;
	cxa_assert(connIn);

	if( prevStateIdIn != CXA_STATE_MACHINE_STATE_UNKNOWN )
	{
		cxa_btle_connection_notify_connectionClose(&connIn->super, connIn->disconnectReason);
	}
}


static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_connection_t *const connIn = (cxa_siLabsBgApi_btle_connection_t *const)userVarIn;
	cxa_assert(connIn);

	bd_addr targetAddr;
	memcpy(targetAddr.addr, connIn->super.targetAddr.bytes, sizeof(targetAddr.addr));

	cxa_eui48_string_t targetAddr_str;
	cxa_eui48_toString(&connIn->super.targetAddr, &targetAddr_str);

	cxa_logger_info(&connIn->logger, "connecting to '%s'", targetAddr_str.str);
	struct gecko_msg_le_gap_connect_rsp_t* rsp = gecko_cmd_le_gap_connect(targetAddr, le_gap_address_type_public, le_gap_phy_1m);
	if( rsp->result != 0 )
	{
		cxa_logger_warn(&connIn->logger, "connection failed: %d", rsp->result);
		connIn->disconnectReason = CXA_BTLE_CONNECTION_DISCONNECT_REASON_STACK;
		cxa_stateMachine_transitionNow(&connIn->stateMachine, STATE_UNUSED);
		return;
	}

	// store our connection handle
	cxa_logger_debug(&connIn->logger, "will be handle %d", rsp->connection);
	connIn->connHandle = rsp->connection;
}


static void stateCb_connectingTimeout_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_connection_t *const connIn = (cxa_siLabsBgApi_btle_connection_t *const)userVarIn;
	cxa_assert(connIn);

	cxa_logger_warn(&connIn->logger, "connection timeout");

	connIn->disconnectReason = CXA_BTLE_CONNECTION_DISCONNECT_REASON_CONNECTION_TIMEOUT;
	cxa_stateMachine_transition(&connIn->stateMachine, STATE_DISCONNECTING);
}


static void stateCb_connIdle_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_connection_t *const connIn = (cxa_siLabsBgApi_btle_connection_t *const)userVarIn;
	cxa_assert(connIn);

	cxa_logger_info(&connIn->logger, "connected and idle");

	connIn->targetProcType = CXA_SILABSBGAPI_PROCTYPE_NONE;
}


static void stateCb_connResolveService_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_connection_t *const connIn = (cxa_siLabsBgApi_btle_connection_t *const)userVarIn;
	cxa_assert(connIn);

	cxa_btle_uuid_t tmpUuid;
	if( !cxa_btle_uuid_initFromString(&tmpUuid, connIn->targetServiceUuid_str) )
	{
		cxa_logger_warn(&connIn->logger, "bad service uuid");
		cxa_stateMachine_transition(&connIn->stateMachine, STATE_CONNECTED_IDLE);
		return;
	}
	cxa_logger_debug(&connIn->logger, "resolving service '%s'", connIn->targetServiceUuid_str);

	// resolve our service
	uint8_t uuidLen;
	uint8_t* uuidBytes;
	if( tmpUuid.type == CXA_BTLE_UUID_TYPE_128BIT )
	{
		cxa_btle_uuid_t uuid_transposed;
		cxa_btle_uuid_initFromUuid(&uuid_transposed, &tmpUuid, true);

		uuidLen = 128 / 8;
		uuidBytes = uuid_transposed.as128Bit.bytes;
	}
	else
	{
		uuidLen = 16 / 8;
		uuidBytes = (uint8_t*)&tmpUuid.as16Bit;
	}

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
	cxa_siLabsBgApi_btle_connection_cachedServiceEntry_t* cachedServiceEntry = getCachedServiceByUuid(connIn, connIn->targetServiceUuid_str);
	if( cachedServiceEntry == NULL )
	{
		cxa_logger_warn(&connIn->logger, "unknown service");
		handleProcedureComplete(connIn, false);
		return;
	}

	cxa_btle_uuid_t tmpUuid;
	if( !cxa_btle_uuid_initFromString(&tmpUuid, connIn->targetCharacteristicUuid_str) )
	{
		cxa_logger_warn(&connIn->logger, "bad char uuid");
		cxa_stateMachine_transition(&connIn->stateMachine, STATE_CONNECTED_IDLE);
		return;
	}
	cxa_logger_debug(&connIn->logger, "resolving characteristic '%s'", connIn->targetCharacteristicUuid_str);

	// resolve our characteristic
	uint8_t uuidLen;
	uint8_t* uuidBytes;
	if( tmpUuid.type == CXA_BTLE_UUID_TYPE_128BIT )
	{
		cxa_btle_uuid_t uuid_transposed;
		cxa_btle_uuid_initFromUuid(&uuid_transposed, &tmpUuid, true);

		uuidLen = 128 / 8;
		uuidBytes = uuid_transposed.as128Bit.bytes;
	}
	else
	{
		uuidLen = 16 / 8;
		uuidBytes = (uint8_t*)&tmpUuid.as16Bit;
	}

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

	cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* cachedCharEntry = getCachedCharacteristicByUuid(connIn, connIn->targetCharacteristicUuid_str);
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

		cxa_btle_connection_notify_readComplete(&connIn->super, connIn->targetServiceUuid_str, connIn->targetCharacteristicUuid_str, false, NULL);
		return;
	}
	// if we made it here, the read was successful
	cxa_logger_debug(&connIn->logger, "read complete, awaiting response");
}


static void stateCb_connWrite_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_connection_t *const connIn = (cxa_siLabsBgApi_btle_connection_t *const)userVarIn;
	cxa_assert(connIn);

	cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* cachedCharEntry = getCachedCharacteristicByUuid(connIn, connIn->targetCharacteristicUuid_str);
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

		cxa_btle_connection_notify_writeComplete(&connIn->super, connIn->targetServiceUuid_str, connIn->targetCharacteristicUuid_str, false);
		return;
	}
	// if we made it here, the write was successful
	cxa_logger_debug(&connIn->logger, "write complete, awaiting response");
}


static void stateCb_connChangeNotiIndi_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_connection_t *const connIn = (cxa_siLabsBgApi_btle_connection_t *const)userVarIn;
	cxa_assert(connIn);

	cxa_siLabsBgApi_btle_connection_cachedCharacteristicEntry_t* cachedCharEntry = getCachedCharacteristicByUuid(connIn, connIn->targetCharacteristicUuid_str);
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

		cxa_btle_connection_notify_notiIndiSubscriptionChanged(&connIn->super, connIn->targetServiceUuid_str, connIn->targetCharacteristicUuid_str, false, false);
		return;
	}
	// if we made it here, the write was successful
	cxa_logger_debug(&connIn->logger, "subscription change requested, awaiting response");
}


static void stateCb_connProcTimeout_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_connection_t *const connIn = (cxa_siLabsBgApi_btle_connection_t *const)userVarIn;
	cxa_assert(connIn);

	cxa_logger_warn(&connIn->logger, "procedure timeout");
	scm_stopConnection(&connIn->super);
}


static void stateCb_disconnecting_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_siLabsBgApi_btle_connection_t *const connIn = (cxa_siLabsBgApi_btle_connection_t *const)userVarIn;
	cxa_assert(connIn);

	cxa_eui48_string_t targetAddr_str;
	cxa_eui48_toString(&connIn->super.targetAddr, &targetAddr_str);

	if( prevStateIdIn == STATE_CONNECTING_TIMEOUT )
	{
		cxa_logger_debug(&connIn->logger, "connection to '%s' failed", targetAddr_str.str);
		cxa_btle_central_notify_connectionStarted((cxa_btle_central_t*)connIn->super.btlec, false, NULL);
	}
	else
	{
		cxa_logger_debug(&connIn->logger, "closing connection to '%s'", targetAddr_str.str);
	}

	gecko_cmd_le_connection_close(connIn->connHandle);
}
