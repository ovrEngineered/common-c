/**
 * Copyright 2016 opencxa.org
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
#include "cxa_uniqueId.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_eui48.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void init(void);
static bool parseIdForInterface(char *const ifaceIn);


// ********  local variable declarations *********
static bool isInit = false;
static uint8_t id_bytes[6];
static char id_str[18];


// ******** global function implementations ********
void cxa_uniqueId_getBytes(uint8_t** bytesOut, size_t* numBytesOut)
{
	if( !isInit ) init();

	if( bytesOut ) *bytesOut = (uint8_t*)&id_bytes;
	if( numBytesOut ) *numBytesOut = sizeof(id_bytes);
}


char* cxa_uniqueId_getHexString(void)
{
	if( !isInit ) init();

	return id_str;
}


// ******** local function implementations ********
static void init(void)
{
	if( parseIdForInterface("eth0") || parseIdForInterface("en0") )
	{
		// we parsed a uniqueId string...get our bytes
		cxa_eui48_t eui48;
		cxa_assert(cxa_eui48_initFromString(&eui48, id_str));

		memcpy(id_bytes, eui48.bytes, sizeof(id_bytes));
	}
	else
	{
		cxa_assert_failWithMsg("failed to parse uniqueId");
	}

	isInit = true;
}


static bool parseIdForInterface(char *const ifaceIn)
{
	cxa_assert(ifaceIn);

	bool retVal = false;
	char targetString[] = "ether ";

	char *returnData = NULL;
	size_t size_bytes = 0;

	char cmdLine[128];
	snprintf(cmdLine, sizeof(cmdLine), "/sbin/ifconfig %s 2>/dev/null", ifaceIn);
	cmdLine[sizeof(cmdLine)-1] = 0;

	FILE *fp = popen(cmdLine, "r");
	while( getline(&returnData, &size_bytes, fp) > 0 )
	{
		if( strstr(returnData, targetString) == NULL )
		{
			free(returnData);
			returnData = NULL;
			continue;
		}
		// if we made it here we have an ethernet line

		// copy to our local store
		char* ethernetAddress_tmp = returnData + strlen(targetString) + 1;
		ethernetAddress_tmp[17] = 0;
		memset(id_str, 0, sizeof(id_str));
		memcpy(id_str, ethernetAddress_tmp, strlen(ethernetAddress_tmp));

		// free memory
		free(returnData);
		returnData = NULL;

		// we're done
		retVal = true;
		break;
	}
	pclose(fp);
	fflush(stdout);

	return retVal;
}
