/**
 * @file
 * @copyright 2017 opencxa.org
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
#ifndef CXA_BTLE_UUID_H_
#define CXA_BTLE_UUID_H_


// ******** includes ********
#include <stdint.h>
#include <cxa_fixedByteBuffer.h>
#include <cxa_uuid128.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_btle_uuid cxa_btle_uuid_t;


typedef enum
{
	CXA_BTLE_UUID_TYPE_16BIT,
	CXA_BTLE_UUID_TYPE_128BIT
}cxa_btle_uuid_type_t;


typedef struct
{
	char str[37];
}cxa_btle_uuid_string_t;


/**
 * @private
 */
struct cxa_btle_uuid
{
	cxa_btle_uuid_type_t type;

	union
	{
		uint16_t as16Bit;
		cxa_uuid128_t as128Bit;
	};
};


// ******** global function prototypes ********
bool cxa_btle_uuid_init(cxa_btle_uuid_t *const uuidIn, uint8_t *const bytesIn, size_t numBytesIn, bool transposeIn);
bool cxa_btle_uuid_initFromBuffer(cxa_btle_uuid_t *const uuidIn, cxa_fixedByteBuffer_t *const fbbIn, size_t indexIn, size_t numBytesIn);
bool cxa_btle_uuid_initFromString(cxa_btle_uuid_t *const uuidIn, const char *const strIn);
void cxa_btle_uuid_initFromUuid(cxa_btle_uuid_t *const targetUuidIn, cxa_btle_uuid_t *const sourceUuidIn, bool transposeIn);

bool cxa_btle_uuid_isEqual(cxa_btle_uuid_t *const uuid1In, cxa_btle_uuid_t *const uuid2In);
bool cxa_btle_uuid_isEqualToString(cxa_btle_uuid_t *const uuid1In, const char *const strIn);

void cxa_btle_uuid_toString(cxa_btle_uuid_t *const uuidIn, cxa_btle_uuid_string_t *const strOut);
void cxa_btle_uuid_toShortString(cxa_btle_uuid_t *const uuidIn, cxa_btle_uuid_string_t *const strOut);

#endif
