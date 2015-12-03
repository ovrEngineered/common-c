/**
 * Copyright 2015 opencxa.org
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
#include "cxa_mqtt_rpc_node_temp.h"


// ******** includes ********
#include <math.h>
#include <cxa_assert.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define DEMO_MIN_C				-15
#define DEMO_MAX_C				15


// ******** local type definitions ********


// ******** local function prototypes ********
static void publishTemp(cxa_mqtt_rpc_node_temp_t *const nodeIn);
static cxa_mqtt_rpc_methodRetVal_t rpcCb_getTemp_c(cxa_mqtt_rpc_node_t *const superIn, cxa_fixedByteBuffer_t *const paramsIn, cxa_fixedByteBuffer_t *const returnParamsOut, void* userVarIn);
static cxa_mqtt_rpc_methodRetVal_t rpcCb_setAlertBounds_c(cxa_mqtt_rpc_node_t *const superIn, cxa_fixedByteBuffer_t *const paramsIn, cxa_fixedByteBuffer_t *const returnParamsOut, void* userVarIn);
static cxa_mqtt_rpc_methodRetVal_t rpcCb_setUpdatePeriod_s(cxa_mqtt_rpc_node_t *const superIn, cxa_fixedByteBuffer_t *const paramsIn, cxa_fixedByteBuffer_t *const returnParamsOut, void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_mqtt_rpc_node_temp_init_subnode(cxa_mqtt_rpc_node_temp_t *const nodeIn, cxa_mqtt_rpc_node_t *const parentNodeIn, char *const nameIn, cxa_timeBase_t *const timeBaseIn)
{
	cxa_assert(nodeIn);
	cxa_assert(parentNodeIn);
	cxa_assert(nameIn);
	cxa_assert(timeBaseIn);

	// initialize our super class
	//cxa_mqtt_rpc_node_init_subNode(&nodeIn->super, parentNodeIn, nameIn);

	// set some initial values
	nodeIn->upperBound_c = HUGE_VALF;
	nodeIn->lowerBound_c = -HUGE_VALF;

	// setup our sample timeDiff
	cxa_timeDiff_init(&nodeIn->td_sample, timeBaseIn, false);

	// setup all of our methods
	//cxa_rpc_node_addMethod(nodeIn->super, "getTemp_c", rpcCb_getTemp_c, NULL);
	//cxa_rpc_node_addMethod(nodeIn->super, "setAlertBounds_c", rpcCb_setAlertBounds_c, NULL);
	//cxa_rpc_node_addMethod(nodeIn->super, "setUpdatePeriod_s", rpcCb_setUpdatePeriod_s, NULL);
}


void cxa_mqtt_rpc_node_temp_update(cxa_mqtt_rpc_node_temp_t * const nodeIn)
{
	cxa_assert(nodeIn);

	if( cxa_timeDiff_isElapsed_recurring_ms(&nodeIn->td_sample, nodeIn->updatePeriod_s * 1000) )
	{
		nodeIn->lastReading_c += 1.0;
		if( nodeIn->lastReading_c > DEMO_MAX_C ) nodeIn->lastReading_c = DEMO_MIN_C;

		publishTemp(nodeIn);
	}
}

// ******** local function implementations ********
static void publishTemp(cxa_mqtt_rpc_node_temp_t *const nodeIn)
{
	cxa_assert(nodeIn);

	if( !cxa_mqtt_rpc_node_publishNotification(&nodeIn->super, "temp_c", CXA_MQTT_QOS_ATMOST_ONCE, &nodeIn->lastReading_c, sizeof(nodeIn->lastReading_c)) )
	{
		cxa_logger_warn(&nodeIn->super.logger, "problem publishising temp");
	}
}


cxa_mqtt_rpc_methodRetVal_t rpcCb_getTemp_c(cxa_mqtt_rpc_node_t *const superIn, cxa_fixedByteBuffer_t *const paramsIn, cxa_fixedByteBuffer_t *const returnParamsOut, void* userVarIn)
{
	cxa_mqtt_rpc_node_temp_t* nodeIn = (cxa_mqtt_rpc_node_temp_t*)superIn;
	cxa_assert(nodeIn);

	// params - none
	if( paramsIn != NULL ) return CXA_MQTT_RPC_METHODRETVAL_FAIL_INVALIDPARAMS;

	if( !cxa_fixedByteBuffer_append_float(returnParamsOut, nodeIn->lastReading_c) ) return CXA_MQTT_RPC_METHODRETVAL_FAIL_INTERNAL;

	return CXA_MQTT_RPC_METHODRETVAL_SUCCESS;
}


cxa_mqtt_rpc_methodRetVal_t rpcCb_setAlertBounds_c(cxa_mqtt_rpc_node_t *const superIn, cxa_fixedByteBuffer_t *const paramsIn, cxa_fixedByteBuffer_t *const returnParamsOut, void* userVarIn)
{
	cxa_mqtt_rpc_node_temp_t* nodeIn = (cxa_mqtt_rpc_node_temp_t*)superIn;
	cxa_assert(nodeIn);

	// params - two floats
	if( !cxa_fixedByteBuffer_get_float(paramsIn, 0, nodeIn->lowerBound_c) ) return CXA_MQTT_RPC_METHODRETVAL_FAIL_INVALIDPARAMS;
	if( !cxa_fixedByteBuffer_get_float(paramsIn, 4, nodeIn->lowerBound_c) ) return CXA_MQTT_RPC_METHODRETVAL_FAIL_INVALIDPARAMS;

	return CXA_MQTT_RPC_METHODRETVAL_SUCCESS;
}


cxa_mqtt_rpc_methodRetVal_t rpcCb_setUpdatePeriod_s(cxa_mqtt_rpc_node_t *const superIn, cxa_fixedByteBuffer_t *const paramsIn, cxa_fixedByteBuffer_t *const returnParamsOut, void* userVarIn)
{
	cxa_mqtt_rpc_node_temp_t* nodeIn = (cxa_mqtt_rpc_node_temp_t*)superIn;
	cxa_assert(nodeIn);

	uint32_t newVal;

	// params - one 32-bit integer
	if( !cxa_fixedByteBuffer_get_uint32BE(paramsIn, 0, newVal) ) return CXA_MQTT_RPC_METHODRETVAL_FAIL_INVALIDPARAMS;

	if( (UINT32_MAX / 1000) >= newVal ) return CXA_MQTT_RPC_METHODRETVAL_FAIL_INVALIDPARAMS;
	nodeIn->updatePeriod_s = newVal;

	return CXA_MQTT_RPC_METHODRETVAL_SUCCESS;
}
