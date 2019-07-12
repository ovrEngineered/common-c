/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_MQTT_RPC_NODE_ROOT_H_
#define CXA_MQTT_RPC_NODE_ROOT_H_


// ******** includes ********
#include <cxa_mqtt_client.h>
#include <cxa_mqtt_rpc_node.h>
#include <cxa_timeDiff.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @private
 */
typedef struct cxa_mqtt_rpc_node_root
{
	cxa_mqtt_rpc_node_t super;

	cxa_mqtt_client_t* mqttClient;
	bool shouldReportState;

	uint16_t currRequestId;
}cxa_mqtt_rpc_node_root_t;


// ******** global function prototypes ********
/**
 * @public
 */
void cxa_mqtt_rpc_node_root_init(cxa_mqtt_rpc_node_root_t *const nodeIn, cxa_mqtt_client_t* const clientIn,
								 bool reportStateIn, const char *nameFmtIn, ...);

#endif // CXA_MQTT_RPC_NODE_ROOT_H_
