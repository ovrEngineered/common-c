/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
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
#ifndef CXA_CONSOLE_COMMAND_BUFFER_LEN_BYTES
	#define CXA_CONSOLE_COMMAND_BUFFER_LEN_BYTES		32
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
void cxa_console_init(const char* deviceNameIn, cxa_ioStream_t *const ioStreamIn, int threadIdIn);

void cxa_console_addCommand(const char* commandIn, const char* descriptionIn,
							cxa_console_argDescriptor_t* argDescsIn, size_t numArgsIn,
							cxa_console_command_cb_t cbIn, void* userVarIn);

void cxa_console_printErrorToIoStream(cxa_ioStream_t *const ioStreamIn, const char *const errorIn);

bool cxa_console_isPaused(void);
void cxa_console_pause(void);
void cxa_console_resume(void);


/**
 * @protected
 */
bool cxa_console_isExecutingCommand(void);
void cxa_console_prelog(void);
void cxa_console_postlog(void);


#endif
