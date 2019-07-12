/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_MQTT_RPC_NODE_BRIDGE_SINGLE_H_
#define CXA_MQTT_RPC_NODE_BRIDGE_SINGLE_H_


// ******** includes ********
#include <cxa_mqtt_rpc_node_bridge.h>
#include <cxa_array.h>
#include <cxa_ioStream.h>
#include <cxa_logger_header.h>
#include <cxa_mqtt_rpc_node.h>
#include <cxa_protocolParser_mqtt.h>
#include <cxa_config.h>


// ******** global macro definitions ********
#ifndef CXA_MQTT_RPC_NODE_BRIDGE_CLIENTID_MAXLEN_BYTES
	#define CXA_MQTT_RPC_NODE_BRIDGE_CLIENTID_MAXLEN_BYTES			17
#endif


// ******** global type definitions *********
typedef struct cxa_mqtt_rpc_node_bridge_single cxa_mqtt_rpc_node_bridge_single_t;


/**
 * @private
 */
struct cxa_mqtt_rpc_node_bridge_single
{
	cxa_mqtt_rpc_node_bridge_t super;

	bool hasClientAuthed;
	char clientId[CXA_MQTT_RPC_NODE_BRIDGE_CLIENTID_MAXLEN_BYTES+1];

	cxa_mqtt_rpc_node_bridge_cb_authenticateClient_t cb_localAuth;
	void* localAuthUserVar;
};


// ******** global function prototypes ********
void cxa_mqtt_rpc_node_bridge_single_init(cxa_mqtt_rpc_node_bridge_single_t *const nodeIn, cxa_mqtt_rpc_node_t *const parentNodeIn,
										  cxa_protocolParser_mqtt_t *const mppIn, const char *nameFmtIn, ...);

void cxa_mqtt_rpc_node_bridge_single_setAuthCb(cxa_mqtt_rpc_node_bridge_single_t *const nodeIn, cxa_mqtt_rpc_node_bridge_cb_authenticateClient_t authCbIn, void *const userVarIn);

#endif // CXA_MQTT_RPC_NODEBRIDGE_SINGLE_H_
