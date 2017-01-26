/**
 * @file
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

void cxa_eui48_toString(cxa_eui48_t *const uuidIn, cxa_eui48_string_t *const strOut);
void cxa_eui48_toShortString(cxa_eui48_t *const uuidIn, cxa_eui48_string_t *const strOut);

#endif
