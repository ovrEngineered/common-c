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
#ifndef CXA_BTLE_ADVPACKET_H_
#define CXA_BTLE_ADVPACKET_H_


// ******** includes ********
#include <stdbool.h>

#include <cxa_btle_uuid.h>
#include <cxa_eui48.h>
#include <cxa_fixedByteBuffer.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_btle_advPacket cxa_btle_advPacket_t;


/**
 * @public
 */
typedef enum
{
	CXA_BTLE_ADVFIELDTYPE_FLAGS = 0x01,
	CXA_BTLE_ADVFIELDTYPE_INCOMPLETE_SERVICE_UUIDS = 0x06,
	CXA_BTLE_ADVFIELDTYPE_COMPLETE_SERVICE_UUIDS = 0x07,
	CXA_BTLE_ADVFIELDTYPE_TXPOWER = 0x0A,
	CXA_BTLE_ADVFIELDTYPE_MAN_DATA = 0xFF
}cxa_btle_advFieldType_t;


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
			cxa_fixedByteBuffer_t uuidBytes;
		}asServiceUuids;

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
 * @private
 */
struct cxa_btle_advPacket
{
	cxa_eui48_t addr;
	bool isRandomAddress;
	int rssi;

	cxa_fixedByteBuffer_t fbb_data;
};


// ******** global function prototypes ********
bool cxa_btle_advPacket_init(cxa_btle_advPacket_t *const advPacketIn,
							 uint8_t *const sourceAddrBytesIn, bool isRandomAddressIn,
							 int rssiIn,
							 uint8_t *const dataIn, size_t dataLen_bytesIn);

cxa_eui48_t* cxa_btle_advPacket_getAddress(cxa_btle_advPacket_t *const advPacketIn);

bool cxa_btle_advPacket_getNumFields(cxa_btle_advPacket_t *const advPacketIn, size_t *const numAdvFieldsOut);
bool cxa_btle_advPacket_getField(cxa_btle_advPacket_t *const advPacketIn, size_t fieldIndexIn, cxa_btle_advField_t *const fieldOut);

bool cxa_btle_advPacket_isAdvertisingService(cxa_btle_advPacket_t *const advPacketIn, const char *const uuidIn);

bool cxa_btle_advField_getNumUuids(cxa_btle_advField_t *const advFieldIn, size_t* numUuidsOut);
bool cxa_btle_advField_getUuid(cxa_btle_advField_t *const advFieldIn, size_t uuidIndexIn, cxa_btle_uuid_t *const uuidOut);

#endif
