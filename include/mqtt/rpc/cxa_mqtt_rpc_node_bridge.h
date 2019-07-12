/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_MQTT_RPC_NODE_BRIDGE_H_
#define CXA_MQTT_RPC_NODE_BRIDGE_H_


// ******** includes ********
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
#ifndef CXA_MQTT_RPC_NODE_BRIDGE_MAPPEDNAME_MAXLEN_BYTES
	#define CXA_MQTT_RPC_NODE_BRIDGE_MAPPEDNAME_MAXLEN_BYTES		9
#endif



// ******** global type definitions *********
typedef struct cxa_mqtt_rpc_node_bridge cxa_mqtt_rpc_node_bridge_t;


typedef enum
{
	CXA_MQTT_RPC_NODE_BRIDGE_AUTH_ALLOW,
	CXA_MQTT_RPC_NODE_BRIDGE_AUTH_DISALLOW,
	CXA_MQTT_RPC_NODE_BRIDGE_AUTH_IGNORE
}cxa_mqtt_rpc_node_bridge_authorization_t;


/**
 * @public
 */
typedef cxa_mqtt_rpc_node_bridge_authorization_t (*cxa_mqtt_rpc_node_bridge_cb_authenticateClient_t)(char *const clientIdIn, size_t clientIdLen_bytes,
																										char *const usernameIn, size_t usernameLen_bytesIn,
																										uint8_t *const passwordIn, size_t passwordLen_bytesIn,
																										void *userVarIn);


/**
 * @private
 */
struct cxa_mqtt_rpc_node_bridge
{
	cxa_mqtt_rpc_node_t super;

	bool isSingle;
	cxa_protocolParser_mqtt_t *mpp;

	cxa_mqtt_rpc_node_bridge_cb_authenticateClient_t cb_auth;
	void* userVar_auth;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_mqtt_rpc_node_bridge_vinit(cxa_mqtt_rpc_node_bridge_t *const nodeIn, cxa_mqtt_rpc_node_t *const parentNodeIn,
										 cxa_protocolParser_mqtt_t *const mppIn,
										 cxa_mqtt_rpc_node_bridge_cb_authenticateClient_t cb_authIn, void* authCbUserVarIn,
										 const char *nameFmtIn, va_list varArgsIn);

#endif // CXA_MQTT_RPC_NODEBRIDGE_H_
