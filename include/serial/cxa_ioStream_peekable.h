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
#ifndef CXA_IOSTREAM_PEEKABLE_H_
#define CXA_IOSTREAM_PEEKABLE_H_


// ******** includes ********
#include <stdbool.h>
#include <cxa_ioStream.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct
{
	cxa_ioStream_t super;
	cxa_ioStream_t* underlyingStream;

	bool hasBufferedByte;
	uint8_t bufferedByte;
}cxa_ioStream_peekable_t;


// ******** global function prototypes ********
void cxa_ioStream_peekable_init(cxa_ioStream_peekable_t *const ioStreamIn,
								cxa_ioStream_t *const underlyingStreamIn);

cxa_ioStream_readStatus_t cxa_ioStream_peekable_hasBytesAvailable(cxa_ioStream_peekable_t *const ioStreamIn);

#endif
