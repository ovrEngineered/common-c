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
								cxa_ioStream_t *const stream2In);


#endif
