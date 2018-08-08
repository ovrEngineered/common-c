/**
 * Copyright 2016 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Christopher Armenio
 */
#include <cxa_lwipMbedTls_network_tcpServer_connectedClient.h>


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_stringUtils.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_INFO
#include <cxa_logger_implementation.h>

#include <lwip/sockets.h>


// ******** local macro definitions ********
#define WRITE_TIMEOUT_MS				2000


// ******** local type definitions ********


// ******** local function prototypes ********
static bool scm_isBound(cxa_network_tcpServer_connectedClient_t *const superIn);
static void scm_unbindAndClose(cxa_network_tcpServer_connectedClient_t *const superIn);
static char* scm_getDescriptiveString(cxa_network_tcpServer_connectedClient_t *const superIn);

static cxa_ioStream_readStatus_t cb_ioStream_readByte(uint8_t *const byteOut, void *const userVarIn);
static bool cb_ioStream_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_lwipMbedTls_network_tcpServer_connectedClient_initUnbound(cxa_lwipMbedTls_network_tcpServer_connectedClient_t *const ccIn)
{
	cxa_assert(ccIn);

	cxa_network_tcpServer_connectedClient_initUnbound(&ccIn->super, scm_isBound, scm_unbindAndClose, scm_getDescriptiveString);

	ccIn->socket = -1;
	cxa_timeDiff_init(&ccIn->td_writeTimeout);
}


void cxa_lwipMbedTls_network_tcpServer_connectedClient_bindToSocket(cxa_lwipMbedTls_network_tcpServer_connectedClient_t *const ccIn,
																	int socketIn,
																	struct sockaddr_in * clientAddressIn)
{
	cxa_assert(ccIn);

	if( cxa_network_tcpServer_connectedClient_isBound(&ccIn->super) ) return;

	ccIn->socket = socketIn;
	cxa_ioStream_bind(&ccIn->super.ioStream, cb_ioStream_readByte, cb_ioStream_writeBytes, (void*)ccIn);

	ccIn->descriptiveString[0] = 0;
	inet_ntop(AF_INET, &clientAddressIn->sin_addr, ccIn->descriptiveString, sizeof(ccIn->descriptiveString));
	cxa_stringUtils_concat_formattedString(ccIn->descriptiveString, sizeof(ccIn->descriptiveString), "::%d", clientAddressIn->sin_port);

	cxa_logger_debug(&ccIn->super.logger, "bound to socket %d", socketIn);
}


// ******** local function implementations ********
static bool scm_isBound(cxa_network_tcpServer_connectedClient_t *const superIn)
{
	cxa_lwipMbedTls_network_tcpServer_connectedClient_t* ccIn = (cxa_lwipMbedTls_network_tcpServer_connectedClient_t*)superIn;
	cxa_assert(ccIn);

	return (ccIn->socket >= 0);
}


static void scm_unbindAndClose(cxa_network_tcpServer_connectedClient_t *const superIn)
{
	cxa_lwipMbedTls_network_tcpServer_connectedClient_t* ccIn = (cxa_lwipMbedTls_network_tcpServer_connectedClient_t*)superIn;
	cxa_assert(ccIn);

	cxa_logger_debug(&ccIn->super.logger, "unbinding and closing");

	cxa_ioStream_unbind(&ccIn->super.ioStream);
	if( ccIn->socket >= 0 ) close(ccIn->socket);
	ccIn->socket = -1;

	// notify our listeners
	cxa_network_tcpServer_connectedClient_notifyDisconnected(&ccIn->super);
}


static char* scm_getDescriptiveString(cxa_network_tcpServer_connectedClient_t *const superIn)
{
	cxa_lwipMbedTls_network_tcpServer_connectedClient_t* ccIn = (cxa_lwipMbedTls_network_tcpServer_connectedClient_t*)superIn;
	cxa_assert(ccIn);

	return ccIn->descriptiveString;
}


static cxa_ioStream_readStatus_t cb_ioStream_readByte(uint8_t *const byteOut, void *const userVarIn)
{
	cxa_lwipMbedTls_network_tcpServer_connectedClient_t* ccIn = (cxa_lwipMbedTls_network_tcpServer_connectedClient_t*)userVarIn;
	cxa_assert(ccIn);

	uint8_t rxByte;
	int rc = recv(ccIn->socket, (void*)&rxByte, 1, 0);
	if( (rc == 0) || ((rc == -1) && (errno != ENOTCONN)) )
	{
		// the connection has been closed
		// per man page: For TCP sockets, the return value 0 means the peer has closed its half side of the connection.
		cxa_logger_debug(&ccIn->super.logger, "connection closed", errno);
		scm_unbindAndClose(&ccIn->super);
		return CXA_IOSTREAM_READSTAT_ERROR;
	}
	if( (rc == -1) && (errno != EAGAIN) )
	{
		// per man page: For TCP sockets, the return value 0 means the peer has closed its half side of the connection.
		// otherwise a -1 and non EAGAIN errno means an error occurred
		cxa_logger_warn(&ccIn->super.logger, "unexpected read error: %d", errno);
		scm_unbindAndClose(&ccIn->super);
		return CXA_IOSTREAM_READSTAT_ERROR;
	}
	// if we made it here one of two things are true: rc==-1 -> no data...rc>0 -> data

	if( (rc > 0) && (byteOut != NULL) ) *byteOut = rxByte;

	return (rc > 0) ? CXA_IOSTREAM_READSTAT_GOTDATA : CXA_IOSTREAM_READSTAT_NODATA;
}


static bool cb_ioStream_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn)
{
	cxa_lwipMbedTls_network_tcpServer_connectedClient_t* ccIn = (cxa_lwipMbedTls_network_tcpServer_connectedClient_t*)userVarIn;
	cxa_assert(ccIn);

	// handle a zero-size buffer appropriately
	if( bufferSize_bytesIn != 0 ) { cxa_assert(buffIn); }
	else { return true; }

	// make sure we are connected
	if( !scm_isBound(&ccIn->super) ) return false;

	// reset our timeout
	cxa_timeDiff_setStartTime_now(&ccIn->td_writeTimeout);

	int tmpRet;
	unsigned char* buf = buffIn;
	do
	{
		tmpRet = send(ccIn->socket, (void*)buf, bufferSize_bytesIn, 0);
		if( tmpRet == 0 )
		{
			// do nothing
		}
		else if( tmpRet > 0 )
		{
			// we made progress...increment our buffer and reset our timeout
			buf += tmpRet;
			bufferSize_bytesIn -= tmpRet;

			cxa_timeDiff_setStartTime_now(&ccIn->td_writeTimeout);
		}
		else if( (tmpRet < 0) && (errno == EAGAIN) )
		{
			// still asking for a write...make sure we don't take too long
			if( cxa_timeDiff_isElapsed_ms(&ccIn->td_writeTimeout, WRITE_TIMEOUT_MS) )
			{
				cxa_logger_warn(&ccIn->super.logger, "timeout during write");
				scm_unbindAndClose(&ccIn->super);
				return false;
			}
		}
		else
		{
			cxa_logger_warn(&ccIn->super.logger, "error during write: %d", tmpRet);
			scm_unbindAndClose(&ccIn->super);
			return false;
		}

	} while( bufferSize_bytesIn > 0 );

	return true;
}
