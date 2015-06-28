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
#include <cxa_linkedField.h>


// ******** global macro definitions ********
#define CXA_RPC_PATH_SEP					"/"
#define CXA_RPC_PATH_UP_ONE_LEVEL			".."
#define CXA_RPC_PATH_GLOBAL_ROOT			"/"
#define CXA_RPC_PATH_LOCAL_ROOT				"~"

#define CXA_RPC_ID_DATATYPE					uint16_t
#define CXA_RPC_ID_MAX						UINT16_MAX


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
struct cxa_rpc_message
{
	cxa_fixedByteBuffer_t* buffer;

	bool areFieldsConfigured;
	cxa_linkedField_t type;
	cxa_linkedField_t dest;
	cxa_linkedField_t method;
	cxa_linkedField_t src;
	cxa_linkedField_t id;
	cxa_linkedField_t params;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_rpc_message_initEmpty(cxa_rpc_message_t *const msgIn, cxa_fixedByteBuffer_t *const fbbIn);
bool cxa_rpc_message_validateReceivedBytes(cxa_rpc_message_t *const msgIn, const size_t startingIndexIn, const size_t len_bytesIn);


/**
 * @public
 */
bool cxa_rpc_message_initRequest(cxa_rpc_message_t *const msgIn, const char *const destIn, const char *const methodIn, uint8_t *const paramsIn, const size_t paramsSize_bytesIn);
bool cxa_rpc_message_initResponse(cxa_rpc_message_t *const msgIn, const char *const reqSrcIn, uint16_t reqIdIn);

cxa_rpc_message_type_t cxa_rpc_message_getType(cxa_rpc_message_t *const msgIn);
char* cxa_rpc_message_getDestination(cxa_rpc_message_t *const msgIn);
char* cxa_rpc_message_getMethod(cxa_rpc_message_t *const msgIn);
char* cxa_rpc_message_getSource(cxa_rpc_message_t *const msgIn);
CXA_RPC_ID_DATATYPE cxa_rpc_message_getId(cxa_rpc_message_t *const msgIn);
cxa_linkedField_t* cxa_rpc_message_getParams(cxa_rpc_message_t *const msgIn);

bool cxa_rpc_message_setId(cxa_rpc_message_t *const msgIn, uint16_t idIn);
bool cxa_rpc_message_prependNodeNameToSource(cxa_rpc_message_t *const msgIn, const char *const nodeNameToPrepend);

bool cxa_rpc_message_destination_getFirstPathComponent(cxa_rpc_message_t *const msgIn, char** pathCompOut, size_t* pathCompLen_bytesOut);
bool cxa_rpc_message_destination_removeFirstPathComponent(cxa_rpc_message_t *const msgIn);

char* cxa_rpc_message_getFriendlyTypeString(cxa_rpc_message_type_t typeIn);

#endif // CXA_RPC_MESSAGE_H_
