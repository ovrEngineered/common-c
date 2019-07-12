/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_NETWORK_FACTORY_H_
#define CXA_NETWORK_FACTORY_H_


// ******** includes ********
#include <cxa_network_tcpClient.h>
#include <cxa_network_tcpServer.h>


// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
cxa_network_tcpClient_t* cxa_network_factory_reserveTcpClient(int threadIdIn);
void cxa_network_factory_freeTcpClient(cxa_network_tcpClient_t *const clientIn);

cxa_network_tcpServer_t* cxa_network_factory_reserveTcpServer(int threadIdIn);
void cxa_network_factory_freeTcpServer(cxa_network_tcpServer_t *const serverIn);

#endif // CXA_NETWORK_FACTORY_H_
