/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_PROTOCOLPARSER_CLE_H_
#define CXA_PROTOCOLPARSER_CLE_H_


// ******** includes ********
#include <cxa_protocolParser.h>
#include <cxa_stateMachine.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct cxa_protocolParser_cleProto cxa_protocolParser_cleProto_t;


struct cxa_protocolParser_cleProto
{
	cxa_protocolParser_t super;

	cxa_stateMachine_t stateMachine;
};


// ******** global function prototypes ********
void cxa_protocolParser_cleProto_init(cxa_protocolParser_cleProto_t *const clePpIn, cxa_ioStream_t *const ioStreamIn, cxa_fixedByteBuffer_t *const buffIn, int threadIdIn);


#endif /* CXA_PROTOCOLPARSER_CLE_H_ */
