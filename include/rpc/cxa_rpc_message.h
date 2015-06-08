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
 * @copyright 2013-2014 opencxa.org
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
#ifndef CXA_RPC_MESSAGE_H_
#define CXA_RPC_MESSAGE_H_


// ******** includes ********
#include <cxa_fixedByteBuffer.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_rpc_message_t object
 */
typedef struct cxa_rpc_message cxa_rpc_message_t;


typedef enum
{
	CXA_RPC_MESSAGE_TYPE_UNKNOWN=0,
	CXA_RPC_MESSAGE_TYPE_REQUEST=1,
	CXA_RPC_MESSAGE_TYPE_RESPONSE=2
}cxa_rpc_message_type_t;


/**
 * @private
 */
struct cxa_rpc_node
{
	cxa_fixedByteBuffer_t *fbbSource;
};


// ******** global function prototypes ********
bool cxa_rpc_message_newRequest(cxa_rpc_message_t *const msgIn, cxa_fixedByteBuffer_t *const fbbIn);
bool cxa_rpc_message_fromBytes(cxa_rpc_message_t *const msgIn, cxa_fixedByteBuffer_t *const fbbIn);


#endif // CXA_RPC_MESSAGE_H_
