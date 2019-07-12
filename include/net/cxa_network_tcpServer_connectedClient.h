/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_NETWORK_TCPSERVER_CONNECTEDCLIENT_H_
#define CXA_NETWORK_TCPSERVER_CONNECTEDCLIENT_H_


// ******** includes ********
#include <cxa_array.h>
#include <cxa_config.h>
#include <cxa_ioStream.h>
#include <cxa_logger_header.h>


// ******** global macro definitions ********
#ifndef CXA_NETWORK_TCPSERVER_CONNECTEDCLIENT_MAXNUM_LISTENERS
#define CXA_NETWORK_TCPSERVER_CONNECTEDCLIENT_MAXNUM_LISTENERS		1
#endif


// ******** global type definitions *********
/**
 * @public
 * Forward declaration of cxa_network_tcpServer_connectedClient_t object
 */
typedef struct cxa_network_tcpServer_connectedClient cxa_network_tcpServer_connectedClient_t;


/**
 * @public
 */
typedef void (*cxa_network_tcpServer_connectedClient_cb_onDisconnect_t)(cxa_network_tcpServer_connectedClient_t *const superIn, void *userVarIn);


/**
 * @protected
 * Used for subclasses
 */
typedef bool (*cxa_network_tcpServer_connectedClient_scm_isBound_t)(cxa_network_tcpServer_connectedClient_t *const superIn);


/**
 * @protected
 * Used for subclasses
 */
typedef void (*cxa_network_tcpServer_connectedClient_scm_unbindAndClose_t)(cxa_network_tcpServer_connectedClient_t *const superIn);


/**
 * @protected
 * Used for subclasses
 */
typedef char* (*cxa_network_tcpServer_connectedClient_scm_getDescriptiveString_t)(cxa_network_tcpServer_connectedClient_t *const superIn);


/**
 * @private
 */
typedef struct
{
	cxa_network_tcpServer_connectedClient_cb_onDisconnect_t cb_onDisconnect;

	void* userVar;
}cxa_network_tcpServer_connectedClient_listenerEntry_t;


/**
 * @private
 */
struct cxa_network_tcpServer_connectedClient
{
	cxa_ioStream_t ioStream;

	cxa_network_tcpServer_connectedClient_scm_isBound_t scm_isBound;
	cxa_network_tcpServer_connectedClient_scm_unbindAndClose_t scm_unbindAndClose;
	cxa_network_tcpServer_connectedClient_scm_getDescriptiveString_t scm_getDescriptiveString;

	cxa_array_t listeners;
	cxa_network_tcpServer_connectedClient_listenerEntry_t listeners_raw[CXA_NETWORK_TCPSERVER_CONNECTEDCLIENT_MAXNUM_LISTENERS];

	cxa_logger_t logger;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_network_tcpServer_connectedClient_initUnbound(cxa_network_tcpServer_connectedClient_t *const ccIn,
							cxa_network_tcpServer_connectedClient_scm_isBound_t scm_isBoundIn,
							cxa_network_tcpServer_connectedClient_scm_unbindAndClose_t scm_unbindAndCloseIn,
							cxa_network_tcpServer_connectedClient_scm_getDescriptiveString_t scm_getDescriptiveStringIn);


/**
 * @public
 */
void cxa_network_tcpServer_connectedClient_addListener(cxa_network_tcpServer_connectedClient_t *const ccIn,
							cxa_network_tcpServer_connectedClient_cb_onDisconnect_t cb_onDisconnectIn,
							void* userVarIn);


/**
 * @public
 */
bool cxa_network_tcpServer_connectedClient_isBound(cxa_network_tcpServer_connectedClient_t *const ccIn);


/**
 * @public
 */
void cxa_network_tcpServer_connectedClient_unbindAndClose(cxa_network_tcpServer_connectedClient_t *const ccIn);


/**
 * @public
 */
cxa_ioStream_t* cxa_network_tcpServer_connectedClient_getIoStream(cxa_network_tcpServer_connectedClient_t *const ccIn);


/**
 * @public
 */
char* cxa_network_tcpServer_connectedClient_getDescriptiveString(cxa_network_tcpServer_connectedClient_t *const ccIn);


/**
 * @protected
 */
void cxa_network_tcpServer_connectedClient_notifyDisconnected(cxa_network_tcpServer_connectedClient_t *const ccIn);


#endif // CXA_NETWORK_TCPSERVER_CONNECTEDCLIENT_H_
