/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_eui48.h"


// ******** includes ********
#include <stdlib.h>
#include <string.h>

#include <cxa_assert.h>
#include <cxa_stringUtils.h>
#include <cxa_timeBase.h>

#define CXA_LOG_LEVEL				CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>

// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_eui48_init(cxa_eui48_t *const uuidIn, uint8_t *const bytesIn)
{
	cxa_assert(uuidIn);
	cxa_assert(bytesIn);

	memcpy(uuidIn->bytes, bytesIn, sizeof(uuidIn->bytes));
}


bool cxa_eui48_initFromBuffer(cxa_eui48_t *const uuidIn, cxa_fixedByteBuffer_t *const fbbIn, size_t indexIn)
{
	cxa_assert(uuidIn);
	cxa_assert(fbbIn);

	return cxa_fixedByteBuffer_get(fbbIn, indexIn, false, uuidIn->bytes, sizeof(uuidIn->bytes));
}


bool cxa_eui48_initFromString(cxa_eui48_t *const uuidIn, const char *const stringIn)
{
	cxa_assert(uuidIn);
	cxa_assert(stringIn);

	size_t strLength_bytes = strlen(stringIn);
	if( strLength_bytes != 17 ) return false;

	char stringWithoutColons[13];
	memset(stringWithoutColons, 0, sizeof(stringWithoutColons));
	uint8_t insertIndex = 0;

	uint8_t numColons = 0;
	for( size_t i = 0; i < strLength_bytes; i++ )
	{
		if( stringIn[i] != ':' )
		{
			stringWithoutColons[insertIndex++] = stringIn[i];
		}
		else
		{
			numColons++;
		}
	}

	if( numColons != 5 ) return false;

	return cxa_stringUtils_hexStringToBytes(stringWithoutColons, sizeof(uuidIn->bytes), true, uuidIn->bytes);
}


void cxa_eui48_initFromEui48(cxa_eui48_t *const targetIn, cxa_eui48_t *const sourceIn)
{
	cxa_assert(targetIn);
	cxa_assert(sourceIn);

	memcpy(targetIn->bytes, sourceIn->bytes, sizeof(targetIn->bytes));
}


void cxa_eui48_initRandom(cxa_eui48_t *const uuidIn)
{
	cxa_assert(uuidIn);

	// per some ESP32 documentation referencing BT documentation
	// A static address is a 48-bit randomly generated address and shall meet the following requirements:
	// • The two most significant bits of the address shall be equal to 1
	// • All bits of the random part of the address shall not be equal to 1
	// • All bits of the random part of the address shall not be equal to 0

	srand(cxa_timeBase_getCount_us());
	for( size_t i = 0; i < sizeof(uuidIn->bytes); i++ )
	{
		// generates [0-255]
		uuidIn->bytes[i] = rand() % (255 + 1 - 0) + 0;
	}

	uuidIn->bytes[0] |= (1 << 7) || (1 << 6);
}


bool cxa_eui48_isEqual(cxa_eui48_t *const uuid1In, cxa_eui48_t *const uuid2In)
{
	cxa_assert(uuid1In);
	cxa_assert(uuid2In);

	return (memcmp(uuid1In->bytes, uuid2In->bytes, sizeof(uuid1In->bytes)) == 0);
}


void cxa_eui48_toString(cxa_eui48_t *const uuidIn, cxa_eui48_string_t *const strOut)
{
	cxa_assert(uuidIn);
	cxa_assert(strOut);

	memset(strOut->str, 0, sizeof(strOut->str));
	char* currStrPtr = strOut->str;

	size_t numBytes = sizeof(uuidIn->bytes);
	for( int i = 0; i < numBytes; i++ )
	{
		if( i != (numBytes - 1) )
		{
			sprintf(currStrPtr, "%02X:", uuidIn->bytes[numBytes-i-1]);
			currStrPtr += 3;
		}
		else
		{
			sprintf(currStrPtr, "%02X", uuidIn->bytes[numBytes-i-1]);
			currStrPtr += 2;
		}
	}
}


bool cxa_eui48_isEqualToString(cxa_eui48_t *const uuidIn, const char *const uuidStrIn)
{
	cxa_assert(uuidIn);
	cxa_assert(uuidStrIn);

	cxa_eui48_t tmpEui48;
	if( !cxa_eui48_initFromString(&tmpEui48, uuidStrIn) ) return false;
	return cxa_eui48_isEqual(uuidIn, &tmpEui48);

}


void cxa_eui48_toShortString(cxa_eui48_t *const uuidIn, cxa_eui48_string_t *const strOut)
{
	cxa_assert(uuidIn);
	cxa_assert(strOut);

	cxa_eui48_string_t tmpStr;
	cxa_eui48_toString(uuidIn, &tmpStr);

	char* lastChars = cxa_stringUtils_getLastCharacters(tmpStr.str, 5);
	if( lastChars == NULL ) lastChars = tmpStr.str;

	cxa_stringUtils_copy(strOut->str, lastChars, sizeof(strOut->str));
}


// ******** local function implementations ********
