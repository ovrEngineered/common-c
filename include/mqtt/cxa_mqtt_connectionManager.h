/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_MQTT_CONN_MAN_H_
#define CXA_MQTT_CONN_MAN_H_


// ******** includes ********
#include <cxa_mqtt_client_network.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef void (*cxa_mqtt_connManager_enteringStandoffCb_t)(void *const userVarIn);
typedef bool (*cxa_mqtt_connManager_canLeaveStandoffCb_t)(void *const userVarIn);


// ******** global function prototypes ********
void cxa_mqtt_connManager_init(char *const hostNameIn, uint16_t portNumIn, int threadIdIn);

void cxa_mqtt_connManager_setStandoffManagementCbs(cxa_mqtt_connManager_enteringStandoffCb_t cb_enteringStandoffIn,
												  cxa_mqtt_connManager_canLeaveStandoffCb_t cb_canLeaveStandoffCbIn,
												  void *const userVarIn);

bool cxa_mqtt_connManager_areCredentialsSet(void);

bool cxa_mqtt_connManager_start(void);
void cxa_mqtt_connManager_stop(void);

uint32_t cxa_mqtt_connManager_getNumFailedConnects(void);

cxa_mqtt_client_t* cxa_mqtt_connManager_getMqttClient(void);


#endif // CXA_MQTT_MAN_H_
