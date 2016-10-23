/**
 * @file
 * @copyright 2016 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Christopher Armenio
 */
#ifndef CONSOLE_CXA_CONSOLE_H_
#define CONSOLE_CXA_CONSOLE_H_


// ******** includes ********
#include <cxa_ioStream.h>
#include <cxa_config.h>


// ******** global macro definitions ********
#ifndef CXA_CONSOLE_ERRMSGLEN_BYTES
	#define CXA_CONSOLE_ERRMSGLEN_BYTES			20
#endif

#ifndef CXA_CONSOLE_MAXNUM_COMMANDS
	#define CXA_CONSOLE_MAXNUM_COMMANDS			10
#endif

#ifndef CXA_CONSOLE_MAX_COMMAND_LEN_BYTES
	#define CXA_CONSOLE_MAX_COMMAND_LEN_BYTES	8
#endif


// ******** global type definitions *********
typedef enum
{
	CXA_CONSOLE_CMDSTAT_SUCCESS,
	CXA_CONSOLE_CMDSTAT_FAIL
}cxa_console_commandStatus_t;


typedef struct
{
	cxa_console_commandStatus_t status;
	char errMsg[CXA_CONSOLE_ERRMSGLEN_BYTES];
}cxa_console_commandRetVal_t;


typedef cxa_console_commandRetVal_t (*cxa_console_command_cb_t)(void* userVarIn);


// ******** global function prototypes ********
void cxa_console_init(const char* deviceNameIn, cxa_ioStream_t *const ioStreamIn);

void cxa_console_addCommand(const char* commandIn, cxa_console_command_cb_t cbIn, void* userVarIn);


/**
 * @protected
 */
void cxa_console_prelog(void);
void cxa_console_postlog(void);


#endif
