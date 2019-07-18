/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_mqtt_rpc_node_posixShell.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_stringUtils.h>

#include <stdio.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_DEBUG
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define MAX_COMMAND_LEN_BYTES				255
#define MAX_COMMAND_STDOUT_LEN_BYTES		255


// ******** local type definitions ********


// ******** local function prototypes ********
static cxa_mqtt_rpc_methodRetVal_t mqttRpcCb_onExecShellCmd(cxa_mqtt_rpc_node_t *const superIn,
															cxa_linkedField_t *const paramsIn, cxa_linkedField_t *const returnParamsOut,
															void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_mqtt_rpc_node_posixShell_init(cxa_mqtt_rpc_node_posixShell_t *const nodeIn, cxa_mqtt_rpc_node_t *const parentNodeIn)
{
	cxa_assert(nodeIn);
	cxa_assert(parentNodeIn);

	// initialize our superclass
	cxa_mqtt_rpc_node_init_formattedString(&nodeIn->super, parentNodeIn, "posixShell");

	// setup our methods
	cxa_mqtt_rpc_node_addMethod(&nodeIn->super, "exec", mqttRpcCb_onExecShellCmd, (void*)nodeIn);
}


// ******** local function implementations ********
static cxa_mqtt_rpc_methodRetVal_t mqttRpcCb_onExecShellCmd(cxa_mqtt_rpc_node_t *const superIn,
															cxa_linkedField_t *const paramsIn, cxa_linkedField_t *const returnParamsOut,
															void* userVarIn)
{
	cxa_mqtt_rpc_node_posixShell_t* nodeIn = (cxa_mqtt_rpc_node_posixShell_t *const)superIn;
	cxa_assert(nodeIn);

	// assuming the payload is the command line, make sure we have _some_ command
	size_t numParamBytes = cxa_linkedField_getSize_bytes(paramsIn);
	char command[MAX_COMMAND_LEN_BYTES];
	memset(command, 0, sizeof(command));

	if( (numParamBytes >= sizeof(command)) ||
		!cxa_linkedField_get(paramsIn, 0, false, (uint8_t*)command, numParamBytes) ||
		(strlen(command) < 1) ) return CXA_MQTT_RPC_METHODRETVAL_FAIL_INVALIDPARAMS;

	cxa_logger_debug(&nodeIn->super.logger, "executing command '%s'", command);

	// execute our command
	char procOutputBuffer[MAX_COMMAND_STDOUT_LEN_BYTES];
	memset(procOutputBuffer, 0, sizeof(procOutputBuffer));
	size_t currOutputBufferIndex = 0;
	FILE* procStdOutStream = popen(command, "r");
	int currChar;
	while( (currChar = fgetc(procStdOutStream)) != EOF )
	{
		procOutputBuffer[currOutputBufferIndex++] = currChar;
		if( currOutputBufferIndex >= (sizeof(procOutputBuffer) - 1) )
		{
			cxa_logger_warn(&nodeIn->super.logger, "command output exceeded buffer length");
		}
	}
	int procExitStatus = pclose(procStdOutStream);
	cxa_stringUtils_trim(procOutputBuffer);

	// get our exit status
	char procExitStatus_str[64];
	memset(procExitStatus_str, 0, sizeof(procExitStatus_str));
	cxa_stringUtils_concat_formattedString(procExitStatus_str, sizeof(procExitStatus_str), "exitStat: %d", procExitStatus);

	// compile our result
//	memset(cxa_linkedField_get_pointerToIndex(returnParamsOut, 0), 0, cxa_linkedField_getMaxSize_bytes(returnParamsOut));
	cxa_linkedField_append_cString(returnParamsOut, procExitStatus_str);
	cxa_linkedField_append_cString(returnParamsOut, "\r\noutput: '");
	cxa_linkedField_append_cString(returnParamsOut, procOutputBuffer);
	cxa_linkedField_append_cString(returnParamsOut, "'");


	return CXA_MQTT_RPC_METHODRETVAL_SUCCESS;
}
