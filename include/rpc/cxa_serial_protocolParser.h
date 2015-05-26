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
#ifndef CXA_SERIAL_PROTOCOLPARSER_H_
#define CXA_SERIAL_PROTOCOLPARSER_H_


// ******** includes ********
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <cxa_array.h>
#include <cxa_fixedByteBuffer.h>
#include <cxa_logger_header.h>
#include <cxa_stateMachine.h>


// ******** global macro definitions ********
#ifndef CXA_SERIAL_PROTOCOLPARSER_MAXNUM_PROTOLISTENERS
	#define CXA_SERIAL_PROTOCOLPARSER_MAXNUM_PROTOLISTENERS		1
#endif
#ifndef CXA_SERIAL_PROTOCOLPARSER_MAXNUM_MSGLISTENERS
	#define CXA_SERIAL_PROTOCOLPARSER_MAXNUM_MSGLISTENERS		1
#endif
#ifndef CXA_SERIAL_PROTOCOLPARSER_MSG_POOL_NUM_MSGS
	#define CXA_SERIAL_PROTOCOLPARSER_MSG_POOL_NUM_MSGS			2
#endif
#ifndef CXA_SERIAL_PROTOCOLPARSER_BUFFER_SIZE_BYTES
	#define CXA_SERIAL_PROTOCOLPARSER_BUFFER_SIZE_BYTES			256
#endif


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_serial_protocolParser_t object
 */
typedef struct cxa_serial_protocolParser cxa_serial_protocolParser_t;


/**
 * @public
 * @brief Callback called when/if a message is received with an unsupported version number.
 * 
 * @param[in] bufferIn a buffer containing the entire message
 * @param[in] userVarIn pointer to the user-supplied variable passed to
 *		::cxa_serial_protocolParser_addProtocolListener
 */
typedef void (*cxa_serial_protocolParser_cb_invalidVersionNumber_t)(cxa_fixedByteBuffer_t *const bufferIn, void *const userVarIn);


/**
 * @public
 * @brief Callback called when/if an error occurs reading/writing to/from the serial device
 *
 * @param[in] userVarIn pointer to the user-supplied variable passed to
 *		::cxa_serial_protocolParser_addProtocolListener
 */
typedef void (*cxa_serial_protocolParser_cb_ioExceptionOccurred_t)(void *const userVarIn);


/**
 * @public
 * @brief Callback called when/if a valid message is received.
 *
 * @param[in] bufferIn a buffer containing the message data
 *		(no header or footer information)
 * @param[in] userVarIn pointer to the user-supplied variable passed to
 *		::cxa_serial_protocolParser_addMessageListener
 */
typedef void (*cxa_serial_protocolParser_cb_messageReceived_t)(cxa_fixedByteBuffer_t *const bufferIn, void *const userVarIn);


/**
 * @private
 */
typedef struct
{
	cxa_serial_protocolParser_cb_invalidVersionNumber_t cb_invalidVer;
	cxa_serial_protocolParser_cb_ioExceptionOccurred_t cb_exception;
	
	void *userVar;
}cxa_serial_protocolParser_protoListener_entry_t;


/**
 * @private
 */
typedef struct
{
	cxa_serial_protocolParser_cb_messageReceived_t cb;
	
	void *userVar;
}cxa_serial_protocolParser_messageListener_entry_t;


/**
 * @private
 */
typedef struct
{
	uint8_t refCount;
	
	cxa_fixedByteBuffer_t buffer;
	uint8_t buffer_raw[CXA_SERIAL_PROTOCOLPARSER_BUFFER_SIZE_BYTES];
}cxa_serial_protocolParser_msgBuffer_t;


/**
 * @private
 */
struct cxa_serial_protocolParser 
{
	FILE *serialDev;
	
	cxa_logger_t logger;
	cxa_array_t protocolListeners;
	cxa_serial_protocolParser_protoListener_entry_t protocolListeners_raw[CXA_SERIAL_PROTOCOLPARSER_MAXNUM_PROTOLISTENERS];
	cxa_array_t messageListeners;
	cxa_serial_protocolParser_messageListener_entry_t messageListeners_raw[CXA_SERIAL_PROTOCOLPARSER_MAXNUM_MSGLISTENERS];
	
	cxa_stateMachine_t stateMachine;
	
	uint8_t userProtoVersion;
	
	
	cxa_fixedByteBuffer_t* currRxBuffer;
	cxa_serial_protocolParser_msgBuffer_t msgPool[CXA_SERIAL_PROTOCOLPARSER_MSG_POOL_NUM_MSGS];
};


// ******** global function prototypes ********
void cxa_serial_protocolParser_init(cxa_serial_protocolParser_t *const sppIn, uint8_t userProtoVersionIn);

void cxa_serial_protocolParser_setSerialDevice(cxa_serial_protocolParser_t *const sppIn, FILE *const fdIn);

cxa_fixedByteBuffer_t* cxa_serial_protocolParser_reserveFreeBuffer(cxa_serial_protocolParser_t *const sppIn);
bool cxa_serial_protocolParser_reserveExistingBuffer(cxa_serial_protocolParser_t *const sppIn, cxa_fixedByteBuffer_t *const dataBytesIn);
void cxa_serial_protocolParser_freeReservedBuffer(cxa_serial_protocolParser_t *const sppIn, cxa_fixedByteBuffer_t *const dataBytesIn);

bool cxa_serial_protocolParser_writeMessage(cxa_serial_protocolParser_t *const sppIn, cxa_fixedByteBuffer_t *const dataBytesIn);

void cxa_serial_protocolParser_addProtocolListener(cxa_serial_protocolParser_t *const sppIn,
	cxa_serial_protocolParser_cb_invalidVersionNumber_t cb_invalidVerIn,
	cxa_serial_protocolParser_cb_ioExceptionOccurred_t cb_exceptionIn,
	void *const userVarIn);

void cxa_serial_protocolParser_addMessageListener(cxa_serial_protocolParser_t *const sppIn,
	cxa_serial_protocolParser_cb_messageReceived_t cb_msgRxIn,
	void *const userVarIn);
	
void cxa_serial_protocolParser_update(cxa_serial_protocolParser_t *const sppIn);


#endif // CXA_SERIAL_PROTOCOLPARSER_H_
