/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_PROTOCOLPARSER_BGAPI_H_
#define CXA_PROTOCOLPARSER_BGAPI_H_


// ******** includes ********
#include <cxa_fixedByteBuffer.h>
#include <cxa_ioStream.h>
#include <cxa_protocolParser.h>
#include <cxa_stateMachine.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct
{
	cxa_protocolParser_t super;

	cxa_stateMachine_t stateMachine;

	size_t rxByteCounter;
}cxa_protocolParser_bgapi_t;


// ******** global function prototypes ********
void cxa_protocolParser_bgapi_init(cxa_protocolParser_bgapi_t *const ppIn, cxa_ioStream_t *const ioStreamIn, cxa_fixedByteBuffer_t *const buffIn, int threadIdIn);

size_t cxa_protocolParser_bgapi_getRxByteCounter(cxa_protocolParser_bgapi_t *const ppIn);

#endif
