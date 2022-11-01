/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_UUID128_H_
#define CXA_UUID128_H_


// ******** includes ********
#include <stdint.h>

#include <cxa_fixedByteBuffer.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct
{
	char str[37];
}cxa_uuid128_string_t;


typedef struct
{
	uint8_t bytes[16];
}cxa_uuid128_t;


// ******** global function prototypes ********
void cxa_uuid128_init(cxa_uuid128_t *const uuidIn, uint8_t *const bytesIn, bool transposeIn);
bool cxa_uuid128_initFromBuffer(cxa_uuid128_t *const uuidIn, cxa_fixedByteBuffer_t *const fbbIn, size_t indexIn, bool transposeBytesIn);
bool cxa_uuid128_initFromString(cxa_uuid128_t *const uuidIn, const char *const stringIn);
void cxa_uuid128_initFromUuid128(cxa_uuid128_t *const targetIn, cxa_uuid128_t *const sourceIn);
void cxa_uuid128_initRandom(cxa_uuid128_t *const uuidIn);
void cxa_uuid128_initEmpty(cxa_uuid128_t *const uuidIn);

bool cxa_uuid128_isEmpty(cxa_uuid128_t *const uuidIn);
bool cxa_uuid128_isEqual(cxa_uuid128_t *const uuid1In, cxa_uuid128_t *const uuid2In);

void cxa_uuid128_toString(cxa_uuid128_t *const uuidIn, cxa_uuid128_string_t *const strOut);
void cxa_uuid128_toShortString(cxa_uuid128_t *const uuidIn, cxa_uuid128_string_t *const strOut);

#endif
