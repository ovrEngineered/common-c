/**
 * Copyright 2013 opencxa.org
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
 */
#include "cxa_logger_implementation.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdarg.h>
#include <string.h>
#include <cxa_assert.h>


// ******** local macro definitions ********
#ifndef CXA_LINE_ENDING
	#define CXA_LINE_ENDING			"\r\n"
#endif


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********
static FILE* fd = NULL;


// ******** global function implementations ********
void cxa_logger_setGlobalFd(FILE *fd_outputIn)
{
	fd = fd_outputIn;
}


void cxa_logger_init(cxa_logger_t *const loggerIn, const char *nameIn)
{
	cxa_assert(loggerIn);
	cxa_assert(nameIn);

	// copy our name (and make sure it will be null terminated)
	strncpy(loggerIn->name, nameIn, CXA_LOGGER_MAX_NAME_LEN_CHARS);
	loggerIn->name[CXA_LOGGER_MAX_NAME_LEN_CHARS-1] = 0;
}


void cxa_logger_vinit(cxa_logger_t *const loggerIn, const char *nameFmtIn, ...)
{
	cxa_assert(loggerIn);
	cxa_assert(nameFmtIn);
	
	va_list varArgs;
	va_start(varArgs, nameFmtIn);
	vsnprintf(loggerIn->name, CXA_LOGGER_MAX_NAME_LEN_CHARS, nameFmtIn, varArgs);
	loggerIn->name[CXA_LOGGER_MAX_NAME_LEN_CHARS-1] = 0;
	va_end(varArgs);
}


void cxa_logger_vlog(cxa_logger_t *const loggerIn, const uint8_t levelIn, const char *formatIn, ...)
{
	cxa_assert(loggerIn);
	cxa_assert( (levelIn == CXA_LOG_LEVEL_ERROR) ||
				(levelIn == CXA_LOG_LEVEL_WARN) ||
				(levelIn == CXA_LOG_LEVEL_INFO) ||
				(levelIn == CXA_LOG_LEVEL_DEBUG) ||
				(levelIn == CXA_LOG_LEVEL_TRACE) );
	cxa_assert(formatIn);

	// figure out our level text
	const char *levelText = "unknown";
	switch( levelIn )
	{
		case CXA_LOG_LEVEL_ERROR:
			levelText = "ERROR";
			break;

		case CXA_LOG_LEVEL_WARN:
			levelText = "WARN";
			break;

		case CXA_LOG_LEVEL_INFO:
			levelText = "INFO";
			break;

		case CXA_LOG_LEVEL_DEBUG:
			levelText = "DEBUG";
			break;

		case CXA_LOG_LEVEL_TRACE:
			levelText = "TRACE";
			break;
	}

	// print the easy part
	fprintf(fd, "%s[%p] %s ", loggerIn->name, loggerIn, levelText);

	// now do our VARARGS
	va_list varArgs;
	va_start(varArgs, formatIn);
	vfprintf(fd, formatIn, varArgs);
	va_end(varArgs);

	// print EOL
	fputs(CXA_LINE_ENDING, fd);
	fflush(fd);
}


// ******** local function implementations ********

