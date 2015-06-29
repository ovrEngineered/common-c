/**
 * Copyright 2013 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef CXA_IOSTREAM_LOOPBACK_H_
#define CXA_IOSTREAM_LOOPBACK_H_


/**
 * @file
 *
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdbool.h>
#include <stdint.h>
#include <cxa_ioStream.h>
#include <cxa_fixedFifo.h>


// ******** global macro definitions ********
#ifndef CXA_IOSTREAM_LOOPBACK_BUFFER_SIZE_BYTES
	#define CXA_IOSTREAM_LOOPBACK_BUFFER_SIZE_BYTES				128
#endif


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_ioStream_loopback_t object
 */
typedef struct cxa_ioStream_loopback cxa_ioStream_loopback_t;


struct cxa_ioStream_loopback
{
	cxa_ioStream_t endPoint1;
	cxa_ioStream_t endPoint2;

	cxa_fixedFifo_t fifo_ep1Read;
	uint8_t fifo_ep1Read_raw[CXA_IOSTREAM_LOOPBACK_BUFFER_SIZE_BYTES];

	cxa_fixedFifo_t fifo_ep2Read;
	uint8_t fifo_ep2Read_raw[CXA_IOSTREAM_LOOPBACK_BUFFER_SIZE_BYTES];
};


// ******** global function prototypes ********
void cxa_ioStream_loopback_init(cxa_ioStream_loopback_t *const ioStreamIn);

cxa_ioStream_t* cxa_ioStream_loopback_getEndpoint1(cxa_ioStream_loopback_t* const ioStreamIn);
cxa_ioStream_t* cxa_ioStream_loopback_getEndpoint2(cxa_ioStream_loopback_t* const ioStreamIn);

#endif // CXA_IOSTREAM_LOOPBACK_H_
