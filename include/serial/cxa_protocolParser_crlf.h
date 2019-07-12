/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_PROTOCOLPARSER_CRLF_H_
#define CXA_PROTOCOLPARSER_CRLF_H_


// ******** includes ********
#include <cxa_protocolParser.h>
#include <cxa_stateMachine.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef struct cxa_protocolParser_crlf cxa_protocolParser_crlf_t;


struct cxa_protocolParser_crlf
{
	cxa_protocolParser_t super;

	bool isPaused;
	cxa_stateMachine_t stateMachine;
};


// ******** global function prototypes ********
/**
 * Starts in the paused state...must call resume for proper operation
 */
void cxa_protocolParser_crlf_init(cxa_protocolParser_crlf_t *const crlfPpIn, cxa_ioStream_t *const ioStreamIn, cxa_fixedByteBuffer_t *const buffIn, int threadIdIn);

void cxa_protocolParser_crlf_pause(cxa_protocolParser_crlf_t *const crlfPpIn);
void cxa_protocolParser_crlf_resume(cxa_protocolParser_crlf_t *const crlfPpIn);


#endif /* CXA_PROTOCOLPARSER_CRLF_H_ */
