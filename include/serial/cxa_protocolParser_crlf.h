/**
 * @file
 *
 * @note This object should work across all architecture-specific implementations
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * @endcode
 *
 *
 * @copyright 2015 opencxa.org
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
