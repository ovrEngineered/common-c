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
#include <cxa_stringUtils.h>
#include <cxa_config.h>


// ******** global macro definitions ********
#ifndef CXA_CONSOLE_ENABLE
#error "Must define CXA_CONSOLE_ENABLE in 'cxa_config.h' to use console"
#endif

#ifndef CXA_CONSOLE_MAXNUM_COMMANDS
	#define CXA_CONSOLE_MAXNUM_COMMANDS				10
#endif

#ifndef CXA_CONSOLE_MAX_COMMAND_LEN_BYTES
	#define CXA_CONSOLE_MAX_COMMAND_LEN_BYTES		16
#endif

#ifndef CXA_CONSOLE_MAX_DESCRIPTION_LEN_BYTES
	#define CXA_CONSOLE_MAX_DESCRIPTION_LEN_BYTES	48
#endif

#ifndef CXA_CONSOLE_MAXNUM_ARGS
	#define CXA_CONSOLE_MAXNUM_ARGS					6
#endif


// ******** global type definitions *********
typedef struct
{
	cxa_stringUtils_dataType_t dataType;
	char description[CXA_CONSOLE_MAX_DESCRIPTION_LEN_BYTES+1];
}cxa_console_argDescriptor_t;


/**
 * @public
 *
 * @param argsIn array of type cxa_stringUtils_parseResult_t
 */
typedef void (*cxa_console_command_cb_t)(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn);


// ******** global function prototypes ********
void cxa_console_init(const char* deviceNameIn, cxa_ioStream_t *const ioStreamIn);

void cxa_console_addCommand(const char* commandIn, const char* descriptionIn,
							cxa_console_argDescriptor_t* argDescsIn, size_t numArgsIn,
							cxa_console_command_cb_t cbIn, void* userVarIn);


/**
 * @protected
 */
void cxa_console_prelog(void);
void cxa_console_postlog(void);


#endif
