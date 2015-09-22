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
#ifndef CXA_PROTOCOLPARSER_H_
#define CXA_PROTOCOLPARSER_H_


// ******** includes ********
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <cxa_config.h>
#include <cxa_array.h>
#include <cxa_ioStream.h>
#include <cxa_logger_header.h>
#include <cxa_stateMachine.h>
#include <cxa_timeBase.h>
#include <cxa_timeDiff.h>


// ******** global macro definitions ********
#ifndef CXA_PROTOCOLPARSER_MAXNUM_PROTOLISTENERS
	#define CXA_PROTOCOLPARSER_MAXNUM_PROTOLISTENERS		1
#endif
#ifndef CXA_PROTOCOLPARSER_MAXNUM_PACKETLISTENERS
	#define CXA_PROTOCOLPARSER_MAXNUM_PACKETLISTENERS		1
#endif


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_protocolParser_t object
 */
typedef struct cxa_protocolParser cxa_protocolParser_t;


/**
 * @public
 * @brief Callback called when/if an error occurs reading/writing to/from the serial device
 *
 * @param[in] userVarIn pointer to the user-supplied variable passed to
 *		::cxa_protocolParser_addProtocolListener
 */
typedef void (*cxa_protocolParser_cb_ioExceptionOccurred_t)(void *const userVarIn);


/**
 * @public
 * @brief Callback called when/if a reception timeout occurs (while receiving a packet)
 *
 * @param[in] incompletePacketIn the contents of the received packet which generated
 * 		the timeout (includes header, data, footer, etc)
 * @param[in] userVarIn pointer to the user-supplied variable passed to
 *		::cxa_protocolParser_addProtocolListener
 */
typedef void (*cxa_protocolParser_cb_receptionTimeout_t)(cxa_fixedByteBuffer_t *const incompletePacketIn, void *const userVarIn);


/**
 * @public
 * @brief Callback called when/if a packet is successfully received
 *
 * @param[in] packetIn a buffer containing the received packet (with header
 * 		and footer stripped and removed from the buffer)
 * @param[in] userVarIn pointer to the user-supplied variable passed to
 *		::cxa_protocolParser_addProtocolListener
 */
typedef void (*cxa_protocolParser_cb_packetReceived_t)(cxa_fixedByteBuffer_t *const packetIn, void *const userVarIn);


/**
 * @private
 */
typedef struct
{
	cxa_protocolParser_cb_ioExceptionOccurred_t cb_exception;
	cxa_protocolParser_cb_receptionTimeout_t cb_receptionTimeout;
	
	void *userVar;
}cxa_protocolParser_protocolListener_entry_t;


/**
 * @private
 */
typedef struct
{
	cxa_protocolParser_cb_packetReceived_t cb;
	
	void *userVar;
}cxa_protocolParser_packetListener_entry_t;


/**
 * @private
 */
struct cxa_protocolParser
{
	cxa_logger_t logger;

	cxa_array_t protocolListeners;
	cxa_protocolParser_protocolListener_entry_t protocolListeners_raw[CXA_PROTOCOLPARSER_MAXNUM_PROTOLISTENERS];

	cxa_array_t packetListeners;
	cxa_protocolParser_packetListener_entry_t packetListeners_raw[CXA_PROTOCOLPARSER_MAXNUM_PACKETLISTENERS];

	bool isReceptionTimeoutEnabled;
	cxa_timeDiff_t td_timeout;

	cxa_stateMachine_t stateMachine;
	cxa_ioStream_t* ioStream;
	
	cxa_fixedByteBuffer_t* currBuffer;
};


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes the protocol parser object
 *
 * @param[in] ppIn pointer to the pre-allocated protocolParser
 * 		to initialize
 * @param[in] ioStreamIn the ioStream on which to send/receive
 * 		packets (may or may not be bound at this point)
 * @param[in] buffIn the initial buffer which should be used to
 * 		receive packets. May be NULL if this will be set later
 * 		(but protocolParser will not operate)
 * @param[in] timeBaseIn the timeBase which shall be used to judge
 * 		reception timeouts (may be null if no timeouts are desired)
 */
void cxa_protocolParser_init(cxa_protocolParser_t *const ppIn, cxa_ioStream_t *const ioStreamIn, cxa_fixedByteBuffer_t *const buffIn,
							 cxa_timeBase_t *const timeBaseIn);

/**
 * @public
 * @brief Adds a listener for exception events
 *
 * @param[in] ppIn pointer to the pre-initialized protocolParser
 * @param[in] cb_exceptionIn callback that should be called when
 * 		an exception occurs within the parser or the underlying
 * 		ioStream. May be NULL.
 * @param[in] cb_receptionTimeoutIn callback tht should be called
 * 		when a reception timeout occurs. A reception timeout will
 * 		occur after a period of inactivity _during_ reception of
 * 		a packet (inactivity after the start of a packet, but before
 * 		the end of the packet). May be NULL.
 * @param[in] userVarIn pointer to a user-supplied variable which
 * 		will be passed to each callback upon execution.
 */
void cxa_protocolParser_addProtocolListener(cxa_protocolParser_t *const ppIn,
		cxa_protocolParser_cb_ioExceptionOccurred_t cb_exceptionIn,
		cxa_protocolParser_cb_receptionTimeout_t cb_receptionTimeoutIn,
		void *const userVarIn);

/**
 * @public
 * @brief Adds a listener for received packets
 *
 * @param[in] ppIn pointer to the pre-initialized protocolParser
 * @param[in] cb_packetRxIn callback that should be called when
 * 		a valid packet is received. May be NULL (why?).
 * @param[in] userVarIn pointer to a user-supplied variable which
 * 		will be passed to the callback upon execution.
 */
void cxa_protocolParser_addPacketListener(cxa_protocolParser_t *const ppIn,
		cxa_protocolParser_cb_packetReceived_t cb_packetRxIn,
		void *const userVarIn);

/**
 * @public
 * @brief Sets the buffer which should be used to receive packets.
 *
 * This function is safe to be called at any time, however, is
 * particularly useful when called during a packetListener callback.
 * This will effectively allow you to create a "ping pong" buffer
 * allowing for extending processing on a received buffer without
 * affecting the ongoing buffering/receive process.
 *
 * @param[in] ppIn pointer to the pre-initialized protocolParser
 * @param[in] buffIn pointer to the buffer which should be used to
 * 		receive incoming packets. May be NULL, in which case, the
 * 		protocolParser will not operate
 */
void cxa_protocolParser_setBuffer(cxa_protocolParser_t *const ppIn, cxa_fixedByteBuffer_t* buffIn);

/**
 * @public
 *
 * @param[in] ppIn pointer to the pre-initialized protocolParser
 *
 * @return the buffer currently being used to receive packets
 */
cxa_fixedByteBuffer_t* cxa_protocolParser_getBuffer(cxa_protocolParser_t *const ppIn);

/**
 * @public
 * @brief Writes a packet to the ioStream
 *
 * @param[in] ppIn pointer to the pre-initialized protocolParser
 * @param[in] dataIn the data to send. Should not include
 * 		header, footer, etc (will be automatically added)
 */
bool cxa_protocolParser_writePacket(cxa_protocolParser_t *const ppIn, cxa_fixedByteBuffer_t* dataIn);

/**
 * @public
 * @brief Updates the protocol parser (and internal state machine).
 * MUST be called on a regular basis for proper operation
 *
 * @param[in] ppIn pointer to the pre-initialized protocolParser
 */
void cxa_protocolParser_update(cxa_protocolParser_t *const ppIn);


#endif // CXA_PROTOCOLPARSER_H_
