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
#include "cxa_logger_implementation.h"


// ******** includes ********
#include <stdarg.h>
#include <string.h>
#include <cxa_assert.h>
#include <cxa_numberUtils.h>
#include <cxa_stringUtils.h>
#include <cxa_timeBase.h>


// ******** local macro definitions ********
#ifndef CXA_LOGGER_BUFFERLEN_BYTES
	#define CXA_LOGGER_BUFFERLEN_BYTES			24
#endif

#define CXA_LOGGER_TRUNCATE_STRING			"..."


// ******** local type definitions ********


// ******** local function prototypes ********
static inline void checkSysLogInit(void);
static void writeField(char *const stringIn, size_t maxFieldLenIn);
static void writeHeader(cxa_logger_t *const loggerIn, const uint8_t levelIn);


// ********  local variable declarations *********
static cxa_logger_t sysLog;
static bool isSysLogInit = false;

static cxa_ioStream_t* ioStream = NULL;
static size_t largestloggerName_bytes = 0;


// ******** global function implementations ********
void cxa_logger_setGlobalIoStream(cxa_ioStream_t *const ioStreamIn)
{
	checkSysLogInit();

	ioStream = ioStreamIn;
	cxa_logger_vlog(&sysLog, CXA_LOG_LEVEL_INFO, "logging ioStream @ %p", ioStreamIn);
}


void cxa_logger_init(cxa_logger_t *const loggerIn, const char *nameIn)
{
	cxa_assert(loggerIn);
	cxa_assert(nameIn);
	checkSysLogInit();

	// copy our name (and make sure it will be null terminated)
	strncpy(loggerIn->name, nameIn, CXA_LOGGER_MAX_NAME_LEN_CHARS);
	loggerIn->name[CXA_LOGGER_MAX_NAME_LEN_CHARS-1] = 0;

	size_t nameLen_bytes = strlen(loggerIn->name);
	if( nameLen_bytes > largestloggerName_bytes ) largestloggerName_bytes = nameLen_bytes;
}


void cxa_logger_vinit(cxa_logger_t *const loggerIn, const char *nameFmtIn, ...)
{
	cxa_assert(loggerIn);
	cxa_assert(nameFmtIn);
	checkSysLogInit();

	va_list varArgs;
	va_start(varArgs, nameFmtIn);
	vsnprintf(loggerIn->name, CXA_LOGGER_MAX_NAME_LEN_CHARS, nameFmtIn, varArgs);
	loggerIn->name[CXA_LOGGER_MAX_NAME_LEN_CHARS-1] = 0;
	va_end(varArgs);

	size_t nameLen_bytes = strlen(loggerIn->name);
	if( nameLen_bytes > largestloggerName_bytes ) largestloggerName_bytes = nameLen_bytes;
}


cxa_logger_t* cxa_logger_getSysLog(void)
{
	return &sysLog;
}


void cxa_logger_log_untermString(cxa_logger_t *const loggerIn, const uint8_t levelIn, const char* prefixIn, const char* untermStringIn, size_t untermStrLen_bytesIn, const char* postFixIn)
{
	cxa_assert(loggerIn);
	cxa_assert(loggerIn);
	cxa_assert( (levelIn == CXA_LOG_LEVEL_ERROR) ||
				(levelIn == CXA_LOG_LEVEL_WARN) ||
				(levelIn == CXA_LOG_LEVEL_INFO) ||
				(levelIn == CXA_LOG_LEVEL_DEBUG) ||
				(levelIn == CXA_LOG_LEVEL_TRACE) );
	cxa_assert(untermStringIn);
	checkSysLogInit();

	// if we don't have an ioStream, don't worry about it!
	if( ioStream == NULL ) return;

	// common header
	writeHeader(loggerIn, levelIn);

	if( prefixIn != NULL ) cxa_ioStream_writeString(ioStream, (char *const)prefixIn);
	cxa_ioStream_writeBytes(ioStream, (void *const)untermStringIn, untermStrLen_bytesIn);
	if( postFixIn != NULL ) cxa_ioStream_writeString(ioStream, (char *const)postFixIn);

	// print EOL
	cxa_ioStream_writeBytes(ioStream, CXA_LINE_ENDING, strlen(CXA_LINE_ENDING));
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
	checkSysLogInit();

	// if we don't have an ioStream, don't worry about it!
	if( ioStream == NULL ) return;

	// common header
	writeHeader(loggerIn, levelIn);

	// our buffer for this go-round
	char buff[CXA_LOGGER_BUFFERLEN_BYTES];

	// now do our VARARGS
	va_list varArgs;
	va_start(varArgs, formatIn);
	int expectedNumBytesWritten = vsnprintf(buff, sizeof(buff), formatIn, varArgs);
	cxa_ioStream_writeBytes(ioStream, buff, CXA_MIN(expectedNumBytesWritten, sizeof(buff)));
	if( sizeof(buff) < expectedNumBytesWritten )
	{
		cxa_ioStream_writeBytes(ioStream, CXA_LOGGER_TRUNCATE_STRING, strlen(CXA_LOGGER_TRUNCATE_STRING));
	}
	va_end(varArgs);

	// print EOL
	cxa_ioStream_writeBytes(ioStream, CXA_LINE_ENDING, strlen(CXA_LINE_ENDING));
}


// ******** local function implementations ********
static inline void checkSysLogInit(void)
{
	if( !isSysLogInit )
	{
		// mark init first since we'll have a stack overflow (recursive call if not)
		isSysLogInit = true;
		cxa_logger_init(&sysLog, "sysLog");
	}
}


static void writeField(char *const stringIn, size_t maxFieldLenIn)
{
	size_t stringLen_bytes = strlen(stringIn);

	if( stringLen_bytes > maxFieldLenIn )
	{
		cxa_ioStream_writeBytes(ioStream, stringIn, maxFieldLenIn-strlen(CXA_LOGGER_TRUNCATE_STRING));
		cxa_ioStream_writeBytes(ioStream, CXA_LOGGER_TRUNCATE_STRING, strlen(CXA_LOGGER_TRUNCATE_STRING));
	}
	else
	{
		cxa_ioStream_writeBytes(ioStream, stringIn, stringLen_bytes);
		for( size_t i = stringLen_bytes; i < maxFieldLenIn; i++ )
		{
			cxa_ioStream_writeByte(ioStream, ' ');
		}
	}
}


static void writeHeader(cxa_logger_t *const loggerIn, const uint8_t levelIn)
{
	cxa_assert(loggerIn);

	// figure out our level text
	char *levelText = "UNKN";
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

	// our buffer for this go-round
	char buff[CXA_LOGGER_BUFFERLEN_BYTES];

	// print the time (if enabled)
	#ifdef CXA_LOGGER_TIME_ENABLE
		snprintf(buff, sizeof(buff), "%-8lX ", cxa_timeBase_getCount_us());
		// 32-bit integer +space
		writeField(buff, 9);
	#endif


	// print the name
	writeField(loggerIn->name, largestloggerName_bytes);

	// pointer (id of logger)
	// 4+ includes [,], optional 0x (depends on printf implementation), terminating space
	snprintf(buff, sizeof(buff), "[%p]", loggerIn);
	writeField(buff, 5+(2*sizeof(void*)));

	// level text
	writeField(levelText, 5);
	cxa_ioStream_writeByte(ioStream, ' ');
}
