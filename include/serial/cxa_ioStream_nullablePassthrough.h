/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_IOSTREAM_NULLABLE_PASSTHROUGH_H_
#define CXA_IOSTREAM_NULLABLE_PASSTHROUGH_H_


// ******** includes ********
#include <cxa_ioStream.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct
{
	cxa_ioStream_t nonnullStream;
	cxa_ioStream_t* nullableStream;

	size_t numBytesWritten;
}cxa_ioStream_nullablePassthrough_t;


// ******** global function prototypes ********
void cxa_ioStream_nullablePassthrough_init(cxa_ioStream_nullablePassthrough_t *const npIn);

cxa_ioStream_t* cxa_ioStream_nullablePassthrough_getNonullStream(cxa_ioStream_nullablePassthrough_t *const npIn);

cxa_ioStream_t* cxa_ioStream_nullablePassthrough_getNullableStream(cxa_ioStream_nullablePassthrough_t *const npIn);
void cxa_ioStream_nullablePassthrough_setNullableStream(cxa_ioStream_nullablePassthrough_t *const npIn,
														cxa_ioStream_t *const ioStreamIn);

size_t cxa_ioStream_nullablePassthrough_getNumBytesWritten(cxa_ioStream_nullablePassthrough_t *const npIn);
void cxa_ioStream_nullablePassthrough_resetNumByesWritten(cxa_ioStream_nullablePassthrough_t *const npIn);


#endif
