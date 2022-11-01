/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_uuid128.h"


// ******** includes ********
#include <stdlib.h>
#include <string.h>

#include <cxa_assert.h>
#include <cxa_stringUtils.h>
#include <cxa_timeBase.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_uuid128_init(cxa_uuid128_t *const uuidIn, uint8_t *const bytesIn, bool transposeIn)
{
	cxa_assert(uuidIn);
	cxa_assert(bytesIn);

	for( size_t i = 0; i < sizeof(uuidIn->bytes); i++ )
	{
		uuidIn->bytes[i] = bytesIn[transposeIn ? (sizeof(uuidIn->bytes)-i-1) : i];
	}
}


bool cxa_uuid128_initFromBuffer(cxa_uuid128_t *const uuidIn, cxa_fixedByteBuffer_t *const fbbIn, size_t indexIn, bool transposeBytesIn)
{
	cxa_assert(uuidIn);
	cxa_assert(fbbIn);

	return cxa_fixedByteBuffer_get(fbbIn, indexIn, transposeBytesIn, uuidIn->bytes, sizeof(uuidIn->bytes));
}


bool cxa_uuid128_initFromString(cxa_uuid128_t *const uuidIn, const char *const stringIn)
{
	cxa_assert(uuidIn);

	// 10250893-25f7-42a0-a756-ea9077377101
	size_t strLength_bytes = strlen(stringIn);
	if( strLength_bytes != 36 ) return false;

	char stringWithoutDashes[33];
	memset(stringWithoutDashes, 0, sizeof(stringWithoutDashes));
	uint8_t insertIndex = 0;

	uint8_t numDashes = 0;
	for( size_t i = 0; i < strLength_bytes; i++ )
	{
		if( stringIn[i] != '-' )
		{
			stringWithoutDashes[insertIndex++] = stringIn[i];
		}
		else
		{
			numDashes++;
		}
	}

	if( numDashes != 4 ) return false;

	return cxa_stringUtils_hexStringToBytes(stringWithoutDashes, sizeof(uuidIn->bytes), false, uuidIn->bytes);
}


void cxa_uuid128_initRandom(cxa_uuid128_t *const uuidIn)
{
	cxa_assert(uuidIn);

	srand(cxa_timeBase_getCount_us());
	for( size_t i = 0; i < sizeof(uuidIn->bytes); i++ )
	{
		// generates [0-255]
		uuidIn->bytes[i] = rand() % (255 + 1 - 0) + 0;
	}
}


void cxa_uuid128_initFromUuid128(cxa_uuid128_t *const targetIn, cxa_uuid128_t *const sourceIn)
{
	cxa_assert(targetIn);
	cxa_assert(sourceIn);

	memcpy(targetIn->bytes, sourceIn->bytes, sizeof(targetIn->bytes));
}


void cxa_uuid128_initEmpty(cxa_uuid128_t *const uuidIn)
{
	cxa_assert(uuidIn);

	memset(uuidIn->bytes, 0, sizeof(uuidIn->bytes));
}


bool cxa_uuid128_isEmpty(cxa_uuid128_t *const uuidIn)
{
	cxa_assert(uuidIn);

	for(size_t i = 0; i < sizeof(uuidIn->bytes); i++ )
	{
		if( uuidIn->bytes[i] != 0 ) return false;
	}
	return true;
}


bool cxa_uuid128_isEqual(cxa_uuid128_t *const uuid1In, cxa_uuid128_t *const uuid2In)
{
	cxa_assert(uuid1In);
	cxa_assert(uuid2In);

	return (memcmp(uuid1In->bytes, uuid2In->bytes, sizeof(uuid1In->bytes)) == 0);
}


void cxa_uuid128_toString(cxa_uuid128_t *const uuidIn, cxa_uuid128_string_t *const strOut)
{
	cxa_assert(uuidIn);
	cxa_assert(strOut);

	memset(strOut->str, 0, sizeof(strOut));

	char* currStrPtr = strOut->str;
	uint8_t* currByte = uuidIn->bytes;

	for( int i = 0; i < 4; i++ )
	{
		sprintf(currStrPtr, "%02X", *currByte);
		currStrPtr += 2;
		currByte++;
	}
	*currStrPtr = '-';
	currStrPtr++;
	for( int i = 0; i < 2; i++ )
	{
		sprintf(currStrPtr, "%02X", *currByte);
		currStrPtr += 2;
		currByte++;
	}
	*currStrPtr = '-';
	currStrPtr++;
	for( int i = 0; i < 2; i++ )
	{
		sprintf(currStrPtr, "%02X", *currByte);
		currStrPtr += 2;
		currByte++;
	}
	*currStrPtr = '-';
	currStrPtr++;
	for( int i = 0; i < 2; i++ )
	{
		sprintf(currStrPtr, "%02X", *currByte);
		currStrPtr += 2;
		currByte++;
	}
	*currStrPtr = '-';
	currStrPtr++;
	for( int i = 0; i < 6; i++ )
	{
		sprintf(currStrPtr, "%02X", *currByte);
		currStrPtr += 2;
		currByte++;
	}
}


void cxa_uuid128_toShortString(cxa_uuid128_t *const uuidIn, cxa_uuid128_string_t *const strOut)
{
	cxa_assert(uuidIn);
	cxa_assert(strOut);

	cxa_uuid128_string_t tmpStr;
	cxa_uuid128_toString(uuidIn, &tmpStr);

	char* lastChars = cxa_stringUtils_getLastCharacters(tmpStr.str, 4);
	if( lastChars == NULL ) lastChars = tmpStr.str;

	cxa_stringUtils_copy(strOut->str, lastChars, sizeof(strOut->str));
}


// ******** local function implementations ********
