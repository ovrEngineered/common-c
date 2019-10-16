/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
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
	uint8_t fbb_data_raw[32];
};


// ******** global function prototypes ********
bool cxa_btle_advPacket_init(cxa_btle_advPacket_t *const advPacketIn,
							 uint8_t *const sourceAddrBytesIn, bool isRandomAddressIn,
							 int rssiIn,
							 uint8_t *const dataIn, size_t dataLen_bytesIn);

void cxa_btle_advPacket_initFromPacket(cxa_btle_advPacket_t *const advPacketIn, cxa_btle_advPacket_t *const sourcePacketIn);

cxa_eui48_t* cxa_btle_advPacket_getAddress(cxa_btle_advPacket_t *const advPacketIn);

bool cxa_btle_advPacket_getNumFields(cxa_btle_advPacket_t *const advPacketIn, size_t *const numAdvFieldsOut);
bool cxa_btle_advPacket_getField(cxa_btle_advPacket_t *const advPacketIn, size_t fieldIndexIn, cxa_btle_advField_t *const fieldOut);

bool cxa_btle_advPacket_isAdvertisingService(cxa_btle_advPacket_t *const advPacketIn, const char *const uuidIn);

bool cxa_btle_advField_getNumUuids(cxa_btle_advField_t *const advFieldIn, size_t* numUuidsOut);
bool cxa_btle_advField_getUuid(cxa_btle_advField_t *const advFieldIn, size_t uuidIndexIn, cxa_btle_uuid_t *const uuidOut);

#endif
