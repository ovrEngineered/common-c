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
}cxa_protocolParser_bgapi_t;


// ******** global function prototypes ********
void cxa_protocolParser_bgapi_init(cxa_protocolParser_bgapi_t *const ppIn, cxa_ioStream_t *const ioStreamIn, cxa_fixedByteBuffer_t *const buffIn, int threadIdIn);


#endif
