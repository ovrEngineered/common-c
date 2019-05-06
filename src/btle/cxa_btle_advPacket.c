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
#include "cxa_btle_advPacket.h"


// ******** includes ********
#include <cxa_assert.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********
typedef enum
{
	NEXT_FIELD_PARSE_RESULT_DONE,
	NEXT_FIELD_PARSE_RESULT_MORE,
	NEXT_FIELD_PARSE_RESULT_INVALID,
}nextFieldParseResult_t;


// ******** local function prototypes ********
static nextFieldParseResult_t getByteIndexOfNextField(cxa_btle_advPacket_t *const advPacketIn, size_t currFieldByteIndexIn, size_t* nextFieldByteIndexOut);
static bool parseAdvField(cxa_btle_advPacket_t *const advPacketIn, cxa_btle_advField_t *advFieldIn, size_t fieldByteIndexIn);


// ********  local variable declarations *********


// ******** global function implementations ********
bool cxa_btle_advPacket_init(cxa_btle_advPacket_t *const advPacketIn,
							 uint8_t *const sourceAddrBytesIn, bool isRandomAddressIn,
							 int rssiIn,
							 uint8_t *const dataIn, size_t dataLen_bytesIn)
{
	cxa_assert(advPacketIn);
	cxa_assert(sourceAddrBytesIn);

	// save our references
	cxa_eui48_init(&advPacketIn->addr, sourceAddrBytesIn);
	advPacketIn->isRandomAddress = isRandomAddressIn;
	advPacketIn->rssi = rssiIn;
	cxa_fixedByteBuffer_init_inPlace(&advPacketIn->fbb_data, dataLen_bytesIn, dataIn, dataLen_bytesIn);

	// make sure we can do some rudimentary parsing of our packet
	return cxa_btle_advPacket_getNumFields(advPacketIn, NULL);
}


bool cxa_btle_advPacket_getNumFields(cxa_btle_advPacket_t *const advPacketIn, size_t *const numAdvFieldsOut)
{
	cxa_assert(advPacketIn);

	int numAdvFields = 0;
	size_t nextAdvFieldByteIndex = 0;
	nextFieldParseResult_t parseResult;
	while( (parseResult = getByteIndexOfNextField(advPacketIn, nextAdvFieldByteIndex, &nextAdvFieldByteIndex)) == NEXT_FIELD_PARSE_RESULT_MORE )
	{
		numAdvFields++;
	}
	if( parseResult != NEXT_FIELD_PARSE_RESULT_DONE ) return false;

	if( numAdvFieldsOut != NULL ) *numAdvFieldsOut = numAdvFields;
	return true;
}


bool cxa_btle_advPacket_getField(cxa_btle_advPacket_t *const advPacketIn, size_t fieldIndexIn, cxa_btle_advField_t *const fieldOut)
{
	cxa_assert(advPacketIn);

	int numAdvFields = 0;
	size_t nextAdvFieldByteIndex = 0;
	nextFieldParseResult_t parseResult;
	do
	{
		if( numAdvFields == fieldIndexIn )
		{
			return parseAdvField(advPacketIn, fieldOut, nextAdvFieldByteIndex);
		}
		numAdvFields++;
	}
	while( (parseResult = getByteIndexOfNextField(advPacketIn, nextAdvFieldByteIndex, &nextAdvFieldByteIndex)) == NEXT_FIELD_PARSE_RESULT_MORE );
	// if we made it here, we've failed

	return false;
}


bool cxa_btle_advPacket_isAdvertisingService(cxa_btle_advPacket_t *const advPacketIn, const char *const uuidIn)
{
	cxa_assert(advPacketIn);

	size_t numAdvFields;
	if( cxa_btle_advPacket_getNumFields(advPacketIn, &numAdvFields) )
	{
		for( size_t currFieldIndex = 0; currFieldIndex < numAdvFields; currFieldIndex++ )
		{
			cxa_btle_advField_t currField;
			if( cxa_btle_advPacket_getField(advPacketIn, currFieldIndex, &currField) &&
				(currField.type == CXA_BTLE_ADVFIELDTYPE_INCOMPLETE_SERVICE_UUIDS) )
			{
				cxa_logger_stepDebug();

				size_t numUuidsThisField;
				if( cxa_btle_advField_getNumUuids(&currField, &numUuidsThisField) )
				{
					for( size_t currUuidIndex = 0; currUuidIndex < numUuidsThisField; currUuidIndex++ )
					{
						cxa_btle_uuid_t currUuid;
						if( cxa_btle_advField_getUuid(&currField, currUuidIndex, &currUuid) )
						{
							if( cxa_btle_uuid_isEqualToString(&currUuid, uuidIn) ) return true;
						}
					}
				}
			}
		}
	}

	return false;
}


bool cxa_btle_advField_getNumUuids(cxa_btle_advField_t *const advFieldIn, size_t* numUuidsOut)
{
	// make sure this is the right type
	if( advFieldIn->type != CXA_BTLE_ADVFIELDTYPE_INCOMPLETE_SERVICE_UUIDS ) return false;

	return cxa_fixedByteBuffer_getSize_bytes(&advFieldIn->asIncompleteServiceUuids.uuidBytes) % 16;
}


bool cxa_btle_advField_getUuid(cxa_btle_advField_t *const advFieldIn, size_t uuidIndexIn, cxa_btle_uuid_t *const uuidOut)
{
	cxa_assert(advFieldIn);

	// make sure this is the right type
	if( advFieldIn->type != CXA_BTLE_ADVFIELDTYPE_INCOMPLETE_SERVICE_UUIDS ) return false;

	// make sure we have enough bytes
	if( cxa_fixedByteBuffer_getSize_bytes(&advFieldIn->asIncompleteServiceUuids.uuidBytes) < ((uuidIndexIn*16) + 16) ) return false;

	// parse it (if needed)
	bool retVal = true;
	if( uuidOut != NULL )
	{
		retVal = cxa_btle_uuid_initFromBuffer(uuidOut, &advFieldIn->asIncompleteServiceUuids.uuidBytes, uuidIndexIn*16, 16);
	}
	return retVal;
}


// ******** local function implementations ********
static nextFieldParseResult_t getByteIndexOfNextField(cxa_btle_advPacket_t *const advPacketIn, size_t currFieldByteIndexIn, size_t* nextFieldByteIndexOut)
{
	cxa_assert(advPacketIn);

	// see if we're done
	if( currFieldByteIndexIn == cxa_fixedByteBuffer_getSize_bytes(&advPacketIn->fbb_data) )
	{
		return NEXT_FIELD_PARSE_RESULT_DONE;
	}

	// try to figure out where the next field is
	uint8_t fieldLen;
	if( (currFieldByteIndexIn > cxa_fixedByteBuffer_getSize_bytes(&advPacketIn->fbb_data)) ||
		!cxa_fixedByteBuffer_get_uint8(&advPacketIn->fbb_data, currFieldByteIndexIn, fieldLen) ||
		(fieldLen == 0) ||
		(fieldLen + currFieldByteIndexIn + 1) > cxa_fixedByteBuffer_getSize_bytes(&advPacketIn->fbb_data) )
	{
		return NEXT_FIELD_PARSE_RESULT_INVALID;
	}

	// return it
	if( nextFieldByteIndexOut != NULL ) *nextFieldByteIndexOut = currFieldByteIndexIn + fieldLen + 1;
	return NEXT_FIELD_PARSE_RESULT_MORE;
}


static bool parseAdvField(cxa_btle_advPacket_t *const advPacketIn, cxa_btle_advField_t *advFieldIn, size_t fieldByteIndexIn)
{
	cxa_assert(advPacketIn);

	if( advFieldIn == NULL ) return true;

	// first byte is always length byte
	uint8_t length_raw;
	if( !cxa_fixedByteBuffer_get_uint8(&advPacketIn->fbb_data, fieldByteIndexIn+0, length_raw) ) return false;
	advFieldIn->length = length_raw;

	// next is type
	uint8_t type_raw;
	if( !cxa_fixedByteBuffer_get_uint8(&advPacketIn->fbb_data, fieldByteIndexIn+1, type_raw) ) return false;
	advFieldIn->type = type_raw;

	// the rest depends on the type
	switch( advFieldIn->type )
	{
		case CXA_BTLE_ADVFIELDTYPE_FLAGS:
		{
			uint8_t flags_raw;
			if( !cxa_fixedByteBuffer_get_uint8(&advPacketIn->fbb_data, fieldByteIndexIn+2, flags_raw) ) return false;
			advFieldIn->asFlags.flags = flags_raw;
			break;
		}

		case CXA_BTLE_ADVFIELDTYPE_TXPOWER:
		{
			uint8_t txPower_raw;
			if( !cxa_fixedByteBuffer_get_uint8(&advPacketIn->fbb_data, fieldByteIndexIn+2, txPower_raw) ) return false;
			advFieldIn->asTxPower.txPower_dBm = txPower_raw;
			break;
		}

		case CXA_BTLE_ADVFIELDTYPE_MAN_DATA:
		{
			uint16_t companyId_raw;
			if( !cxa_fixedByteBuffer_get_uint16LE(&advPacketIn->fbb_data, fieldByteIndexIn+2, companyId_raw) ) return false;
			advFieldIn->asManufacturerData.companyId = companyId_raw;

			size_t manDataSize_bytes = advFieldIn->length-3;
			cxa_fixedByteBuffer_init_subBufferFixedSize(&advFieldIn->asManufacturerData.manBytes, &advPacketIn->fbb_data, fieldByteIndexIn+4, manDataSize_bytes);
			break;
		}

		case CXA_BTLE_ADVFIELDTYPE_INCOMPLETE_SERVICE_UUIDS:
		{
			size_t uuidSize_bytes = advFieldIn->length-1;
			cxa_fixedByteBuffer_init_subBufferFixedSize(&advFieldIn->asIncompleteServiceUuids.uuidBytes, &advPacketIn->fbb_data, fieldByteIndexIn+2, uuidSize_bytes);
			break;
		}

		default:
		{
//			cxa_logger_stepDebug_msg("unknown adv field type: %d", type_raw);
			break;
		}
	}
	return true;
}
