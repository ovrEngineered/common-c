/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_MQTT_RPC_NODE_POSIXSHELL_H_
#define CXA_MQTT_RPC_NODE_POSIXSHELL_H_


// ******** includes ********
#include <cxa_mqtt_rpc_node.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_mqtt_rpc_node_posixShell cxa_mqtt_rpc_node_posixShell_t;


/**
 * @private
 */
struct cxa_mqtt_rpc_node_posixShell
{
	cxa_mqtt_rpc_node_t super;
};


// ******** global function prototypes ********
void cxa_mqtt_rpc_node_posixShell_init(cxa_mqtt_rpc_node_posixShell_t *const nodeIn, cxa_mqtt_rpc_node_t *const parentNodeIn);


#endif
