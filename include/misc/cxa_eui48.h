/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_eui48_H_
#define CXA_eui48_H_


// ******** includes ********
#include <stdint.h>

#include <cxa_fixedByteBuffer.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct
{
	char str[18];
}cxa_eui48_string_t;


typedef struct
{
	uint8_t bytes[6];
}cxa_eui48_t;


// ******** global function prototypes ********
void cxa_eui48_init(cxa_eui48_t *const uuidIn, uint8_t *const bytesIn);
bool cxa_eui48_initFromBuffer(cxa_eui48_t *const uuidIn, cxa_fixedByteBuffer_t *const fbbIn, size_t indexIn);
bool cxa_eui48_initFromString(cxa_eui48_t *const uuidIn, const char *const stringIn);
void cxa_eui48_initFromEui48(cxa_eui48_t *const targetIn, cxa_eui48_t *const sourceIn);
void cxa_eui48_initRandom(cxa_eui48_t *const uuidIn);

bool cxa_eui48_isEqual(cxa_eui48_t *const uuid1In, cxa_eui48_t *const uuid2In);
bool cxa_eui48_isEqualToString(cxa_eui48_t *const uuidIn, const char *const uuidStrIn);

void cxa_eui48_toString(cxa_eui48_t *const uuidIn, cxa_eui48_string_t *const strOut);
void cxa_eui48_toShortString(cxa_eui48_t *const uuidIn, cxa_eui48_string_t *const strOut);

#endif
