/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
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
