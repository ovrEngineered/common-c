/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_NETWORK_TCPSERVER_H_
#define CXA_NETWORK_TCPSERVER_H_


// ******** includes ********
#include <stdbool.h>
#include <cxa_array.h>
#include <cxa_ioStream.h>
#include <cxa_network_tcpServer_connectedClient.h>
#include <cxa_timeDiff.h>
#include <cxa_logger_header.h>
#include <cxa_config.h>


// ******** global macro definitions ********
#ifndef CXA_NETWORK_TCPSERVER_MAXNUM_LISTENERS
	#define CXA_NETWORK_TCPSERVER_MAXNUM_LISTENERS		1
#endif


// ******** global type definitions *********
/**
 * @public
 * Forward declaration of cxa_network_tcpServer_t object
 */
typedef struct cxa_network_tcpServer cxa_network_tcpServer_t;


/**
 * @public
 */
typedef void (*cxa_network_tcpServer_cb_onConnect_t)(cxa_network_tcpServer_t *const serverIn, cxa_network_tcpServer_connectedClient_t* clientIn, void* userVarIn);


/**
 * @protected
 * Used for subclasses
 */
typedef bool (*cxa_network_tcpServer_scm_listen_t)(cxa_network_tcpServer_t *const superIn, uint16_t portNumIn);


/**
 * @protected
 * Used for subclasses
 */
typedef void (*cxa_network_tcpServer_scm_stopListening_t)(cxa_network_tcpServer_t *const superIn);


/**
 * @private
 */
typedef struct
{
	cxa_network_tcpServer_cb_onConnect_t cb_onConnect;

	void* userVar;
}cxa_network_tcpServer_listenerEntry_t;


/**
 * @private
 */
struct cxa_network_tcpServer
{
	// subclass methods
	cxa_network_tcpServer_scm_listen_t scm_listen;
	cxa_network_tcpServer_scm_stopListening_t scm_stopListening;

	cxa_array_t listeners;
	cxa_network_tcpServer_listenerEntry_t listeners_raw[CXA_NETWORK_TCPSERVER_MAXNUM_LISTENERS];

	cxa_logger_t logger;
};


// ******** global function prototypes ********
/**
 * @private
 */
void cxa_network_tcpServer_init(cxa_network_tcpServer_t *const tcpServerIn,
								cxa_network_tcpServer_scm_listen_t scm_listenIn,
								cxa_network_tcpServer_scm_stopListening_t scm_stopListeningIn);

/**
 * @public
 */
void cxa_network_tcpServer_addListener(cxa_network_tcpServer_t *const tcpServerIn,
									   cxa_network_tcpServer_cb_onConnect_t cb_onConnectIn,
									   void* userVarIn);


/**
 * @public
 */
bool cxa_network_tcpServer_listen(cxa_network_tcpServer_t *const tcpServerIn, uint16_t portNumIn);


/**
 * @public
 */
void cxa_network_tcpServer_stopListening(cxa_network_tcpServer_t *const tcpServerIn);


/**
 * @protected
 */
void cxa_network_tcpServer_notifyConnect(cxa_network_tcpServer_t *const tcpServerIn, cxa_network_tcpServer_connectedClient_t* clientIn);


#endif // CXA_NETWORK_TCPSERVER_H_
