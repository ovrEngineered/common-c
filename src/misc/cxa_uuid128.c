/**
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
void cxa_uuid128_init(cxa_uuid128_t *const uuidIn, uint8_t *const bytesIn)
{
	cxa_assert(uuidIn);
	cxa_assert(bytesIn);

	memcpy(uuidIn->bytes, bytesIn, sizeof(uuidIn->bytes));
}


bool cxa_uuid128_initFromBuffer(cxa_uuid128_t *const uuidIn, cxa_fixedByteBuffer_t *const fbbIn, size_t indexIn)
{
	cxa_assert(uuidIn);
	cxa_assert(fbbIn);

	return cxa_fixedByteBuffer_get(fbbIn, indexIn, true, uuidIn->bytes, sizeof(uuidIn->bytes));
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

	return cxa_stringUtils_hexStringToBytes(stringWithoutDashes, sizeof(uuidIn->bytes), true, uuidIn->bytes);
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

	cxa_uuid128_toString(uuidIn, strOut);
	strOut->str[7] = 0;
}


// ******** local function implementations ********
