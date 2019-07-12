/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_IOSTREAM_LOOPBACK_H_
#define CXA_IOSTREAM_LOOPBACK_H_


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
	cxa_ioStream_t super;

	cxa_fixedFifo_t fifo;
	uint8_t fifo_raw[CXA_IOSTREAM_LOOPBACK_BUFFER_SIZE_BYTES];
};


// ******** global function prototypes ********
void cxa_ioStream_loopback_init(cxa_ioStream_loopback_t *const ioStreamIn);

#endif // CXA_IOSTREAM_LOOPBACK_H_
