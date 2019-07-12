/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_IOSTREAM_TEE_H_
#define CXA_IOSTREAM_TEE_H_


// ******** includes ********
#include <stdbool.h>
#include <stdint.h>
#include <cxa_ioStream.h>
#include <cxa_fixedFifo.h>


// ******** global macro definitions ********
#ifndef CXA_IOSTREAM_TEE_BUFFER_SIZE_BYTES
	#define CXA_IOSTREAM_TEE_BUFFER_SIZE_BYTES				128
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef struct
{
	cxa_ioStream_t endPoint1;
	cxa_ioStream_t endPoint2;
	cxa_ioStream_t endPoint3;

	cxa_fixedFifo_t fifo_ep1Read;
	uint8_t fifo_ep1Read_raw[CXA_IOSTREAM_TEE_BUFFER_SIZE_BYTES];

	cxa_fixedFifo_t fifo_ep2Read;
	uint8_t fifo_ep2Read_raw[CXA_IOSTREAM_TEE_BUFFER_SIZE_BYTES];

	cxa_fixedFifo_t fifo_ep3Read;
	uint8_t fifo_ep3Read_raw[CXA_IOSTREAM_TEE_BUFFER_SIZE_BYTES];
}cxa_ioStream_tee_t;


// ******** global function prototypes ********
void cxa_ioStream_tee_init(cxa_ioStream_tee_t *const ioStreamIn);

cxa_ioStream_t* cxa_ioStream_tee_getEndpoint1(cxa_ioStream_tee_t* const ioStreamIn);
cxa_ioStream_t* cxa_ioStream_tee_getEndpoint2(cxa_ioStream_tee_t* const ioStreamIn);
cxa_ioStream_t* cxa_ioStream_tee_getEndpoint3(cxa_ioStream_tee_t* const ioStreamIn);

#endif // CXA_IOSTREAM_TEE_H_
