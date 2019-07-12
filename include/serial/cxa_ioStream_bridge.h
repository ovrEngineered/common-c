/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_IOSTREAM_BRIDGE_H_
#define CXA_IOSTREAM_BRIDGE_H_


// ******** includes ********
#include <cxa_ioStream.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct
{
	cxa_ioStream_t* stream1;
	cxa_ioStream_t* stream2;
}cxa_ioStream_bridge_t;


// ******** global function prototypes ********
void cxa_ioStream_bridge_init(cxa_ioStream_bridge_t *const bridgeIn,
								cxa_ioStream_t *const stream1In,
								cxa_ioStream_t *const stream2In,
								int threadIdIn);


#endif
