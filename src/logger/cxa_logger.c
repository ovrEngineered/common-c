/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_logger_implementation.h"


// ******** includes ********
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <cxa_assert.h>
#include <cxa_config.h>
#include <cxa_mutex.h>
#include <cxa_numberUtils.h>
#include <cxa_stringUtils.h>
#include <cxa_timeBase.h>

#ifdef CXA_CONSOLE_ENABLE
#include <cxa_console.h>
#endif


// ******** local macro definitions ********
#define CXA_LOGGER_TRUNCATE_STRING			"..."


// ******** local type definitions ********


// ******** local function prototypes ********
static inline void checkInit(void);
static void cxa_logger_log_varArgs(cxa_logger_t *const loggerIn, const uint8_t levelIn, const char* formatIn, va_list argsIn);
static void writeField(const char *const stringIn, size_t maxFieldLenIn);
static void writeHeader(cxa_logger_t *const loggerIn, const uint8_t levelIn);


// ********  local variable declarations *********
static cxa_logger_t sysLog;
static bool isInit = false;

static cxa_ioStream_t* ioStream = NULL;
static size_t largestloggerName_bytes = 0;
static cxa_mutex_t* printMutex;


// ******** global function implementations ********
void cxa_logger_setGlobalIoStream(cxa_ioStream_t *const ioStreamIn)
{
	checkInit();

	ioStream = ioStreamIn;

	cxa_ioStream_writeBytes(ioStream, (void*)CXA_LINE_ENDING, sizeof(CXA_LINE_ENDING));
	cxa_ioStream_writeBytes(ioStream, (void*)CXA_LINE_ENDING, sizeof(CXA_LINE_ENDING));
	cxa_logger_log_formattedString_impl(&sysLog, CXA_LOG_LEVEL_INFO, "logging ioStream @ %p", ioStreamIn);
}


void cxa_logger_init(cxa_logger_t *const loggerIn, const char *nameIn)
{
	cxa_assert(loggerIn);
	cxa_assert(nameIn);
	checkInit();

	// copy our name (and make sure it will be null terminated)
	cxa_stringUtils_copy(loggerIn->name, nameIn, CXA_LOGGER_MAX_NAME_LEN_CHARS);
	loggerIn->name[CXA_LOGGER_MAX_NAME_LEN_CHARS-1] = 0;

	size_t nameLen_bytes = strlen(loggerIn->name);
	if( nameLen_bytes > largestloggerName_bytes ) largestloggerName_bytes = nameLen_bytes;

	#ifdef CXA_LOGGER_CLAMPED_ENABLE
	cxa_timeDiff_init(&loggerIn->td_clamped);
	#endif
}


void cxa_logger_init_formattedString(cxa_logger_t *const loggerIn, const char *nameFmtIn, ...)
{
	cxa_assert(loggerIn);
	cxa_assert(nameFmtIn);
	checkInit();

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
	checkInit();

	return &sysLog;
}


void cxa_logger_log_formattedString_impl(cxa_logger_t *const loggerIn, const uint8_t levelIn, const char* formatIn, ...)
{
	cxa_assert(loggerIn);
	cxa_assert(formatIn);
	checkInit();

	va_list varArgs;
	va_start(varArgs, formatIn);
	cxa_logger_log_varArgs(loggerIn, levelIn, formatIn, varArgs);
	va_end(varArgs);
}


void cxa_logger_log_untermString_impl(cxa_logger_t *const loggerIn, const uint8_t levelIn, const char* prefixIn, const char* untermStringIn, size_t untermStrLen_bytesIn, const char* postFixIn)
{
	cxa_assert(loggerIn);
	cxa_assert(loggerIn);
	cxa_assert( (levelIn == CXA_LOG_LEVEL_ERROR) ||
				(levelIn == CXA_LOG_LEVEL_WARN) ||
				(levelIn == CXA_LOG_LEVEL_INFO) ||
				(levelIn == CXA_LOG_LEVEL_DEBUG) ||
				(levelIn == CXA_LOG_LEVEL_TRACE) );
	cxa_assert(untermStringIn);
	checkInit();

	// if we don't have an ioStream, don't worry about it!
	if( ioStream == NULL ) return;


	cxa_mutex_aquire(printMutex);

#ifdef CXA_CONSOLE_ENABLE
	cxa_console_prelog();
	if( cxa_console_isExecutingCommand() )
	{
		cxa_mutex_release(printMutex);
		return;
	}
#endif

	// common header
	writeHeader(loggerIn, levelIn);

	if( prefixIn != NULL ) cxa_ioStream_writeString(ioStream, (char *const)prefixIn);
	cxa_ioStream_writeBytes(ioStream, (void *const)untermStringIn, untermStrLen_bytesIn);
	if( postFixIn != NULL ) cxa_ioStream_writeString(ioStream, (char *const)postFixIn);

	// print EOL
	cxa_ioStream_writeString(ioStream, CXA_LINE_ENDING);

#ifdef CXA_CONSOLE_ENABLE
	cxa_console_postlog();
#endif

	cxa_mutex_release(printMutex);
}


void cxa_logger_log_memdump_impl(cxa_logger_t *const loggerIn, const uint8_t levelIn, const char* prefixIn, const void* ptrIn, size_t ptrLen_bytes, const char* postFixIn)
{
	cxa_assert(loggerIn);
	cxa_assert(loggerIn);
	cxa_assert( (levelIn == CXA_LOG_LEVEL_ERROR) ||
			(levelIn == CXA_LOG_LEVEL_WARN) ||
			(levelIn == CXA_LOG_LEVEL_INFO) ||
			(levelIn == CXA_LOG_LEVEL_DEBUG) ||
			(levelIn == CXA_LOG_LEVEL_TRACE) );
	cxa_assert(ptrIn);
	checkInit();

	// if we don't have an ioStream, don't worry about it!
	if( ioStream == NULL ) return;


	cxa_mutex_aquire(printMutex);

#ifdef CXA_CONSOLE_ENABLE
	cxa_console_prelog();
	if( cxa_console_isExecutingCommand() )
	{
		cxa_mutex_release(printMutex);
		return;
	}
#endif

	// common header
	writeHeader(loggerIn, levelIn);

	// write our message
	if( prefixIn != NULL ) cxa_ioStream_writeString(ioStream, (char *const)prefixIn);
	cxa_ioStream_writeString(ioStream, "{");
	for( size_t i = 0; i < ptrLen_bytes; i++ )
	{
		cxa_ioStream_writeFormattedString(ioStream, "%02X", ((uint8_t*)ptrIn)[i]);
		if( i != (ptrLen_bytes-1) ) cxa_ioStream_writeString(ioStream, ", ");
	}
	cxa_ioStream_writeString(ioStream, "}");
	if( postFixIn != NULL ) cxa_ioStream_writeString(ioStream, (char *const)postFixIn);

	// print EOL
	cxa_ioStream_writeString(ioStream, CXA_LINE_ENDING);

#ifdef CXA_CONSOLE_ENABLE
	cxa_console_postlog();
#endif

	cxa_mutex_release(printMutex);
}


void cxa_logger_stepDebug_formattedString_impl(const char* fileIn, const int lineNumIn, const char* formatIn, ...)
{
	cxa_assert(fileIn);
	checkInit();

	// if we don't have an ioStream, don't worry about it!
	if( ioStream == NULL ) return;

	// shorten our file name
	char *file_sep = strrchr(fileIn, '/');
	if(file_sep) fileIn = file_sep+1;
	else{
		file_sep = strrchr(fileIn, '\\');
		if (file_sep) fileIn = file_sep+1;
	}


	cxa_mutex_aquire(printMutex);

#ifdef CXA_CONSOLE_ENABLE
	cxa_console_prelog();
#endif

	// common header
	writeHeader(&sysLog, CXA_LOG_LEVEL_DEBUG);

	// print our location
	cxa_ioStream_writeFormattedString(ioStream, ((formatIn != NULL) ? "%s::%d - " : "%s::%d"), fileIn, lineNumIn);

	// now do our VARARGS
	if( formatIn != NULL )
	{
		va_list varArgs;
		va_start(varArgs, formatIn);
		cxa_ioStream_vWriteString(ioStream, formatIn, varArgs, true, CXA_LOGGER_TRUNCATE_STRING);
		va_end(varArgs);
	}

	// print EOL
	cxa_ioStream_writeBytes(ioStream, (void*)CXA_LINE_ENDING, strlen(CXA_LINE_ENDING));

#ifdef CXA_CONSOLE_ENABLE
	cxa_console_postlog();
#endif

	cxa_mutex_release(printMutex);
}


void cxa_logger_stepDebug_memDump_impl(const char* fileIn, const int lineNumIn, void* bytesIn, size_t numBytesIn, const char* msgIn)
{
	cxa_assert(fileIn);
	cxa_assert(bytesIn);
	checkInit();

	// if we don't have an ioStream, don't worry about it!
	if( ioStream == NULL ) return;

	// shorten our file name
	char *file_sep = strrchr(fileIn, '/');
	if(file_sep) fileIn = file_sep+1;
	else{
		file_sep = strrchr(fileIn, '\\');
		if (file_sep) fileIn = file_sep+1;
	}


	cxa_mutex_aquire(printMutex);

#ifdef CXA_CONSOLE_ENABLE
	cxa_console_prelog();
#endif

	// common header
	writeHeader(&sysLog, CXA_LOG_LEVEL_DEBUG);

	// print our location
	cxa_ioStream_writeFormattedString(ioStream,  "%s::%d - ", fileIn, lineNumIn);

	// print our message
	cxa_ioStream_writeString(ioStream, msgIn);

	cxa_ioStream_writeString(ioStream, "{");
	for( size_t i = 0; i < numBytesIn; i++ )
	{
		cxa_ioStream_writeFormattedString(ioStream, "%02X", ((uint8_t*)bytesIn)[i]);
		if( i != (numBytesIn-1) ) cxa_ioStream_writeString(ioStream, ", ");
	}
	cxa_ioStream_writeString(ioStream, "}");


	// print EOL
	cxa_ioStream_writeBytes(ioStream, (void*)CXA_LINE_ENDING, strlen(CXA_LINE_ENDING));

#ifdef CXA_CONSOLE_ENABLE
	cxa_console_postlog();
#endif

	cxa_mutex_release(printMutex);
}


// ******** local function implementations ********
void cxa_logger_log_varArgs(cxa_logger_t *const loggerIn, const uint8_t levelIn, const char* formatIn, va_list argsIn)
{
	cxa_assert(loggerIn);
	cxa_assert( (levelIn == CXA_LOG_LEVEL_ERROR) ||
				(levelIn == CXA_LOG_LEVEL_WARN) ||
				(levelIn == CXA_LOG_LEVEL_INFO) ||
				(levelIn == CXA_LOG_LEVEL_DEBUG) ||
				(levelIn == CXA_LOG_LEVEL_TRACE) );
	cxa_assert(formatIn);
	checkInit();

	// if we don't have an ioStream, don't worry about it!
	if( ioStream == NULL ) return;


	cxa_mutex_aquire(printMutex);

#ifdef CXA_CONSOLE_ENABLE
	cxa_console_prelog();
	if( cxa_console_isExecutingCommand() )
	{
		cxa_mutex_release(printMutex);
		return;
	}
#endif

	// common header
	writeHeader(loggerIn, levelIn);

	// now do our VARARGS
	cxa_ioStream_vWriteString(ioStream, formatIn, argsIn, true, CXA_LOGGER_TRUNCATE_STRING);

	// print EOL
	cxa_ioStream_writeBytes(ioStream, (void*)CXA_LINE_ENDING, strlen(CXA_LINE_ENDING));

#ifdef CXA_CONSOLE_ENABLE
	cxa_console_postlog();
#endif

	cxa_mutex_release(printMutex);
}


static inline void checkInit(void)
{
	if( !isInit )
	{
		// mark init first since we'll have a stack overflow (recursive call if not)
		isInit = true;
		cxa_assert(printMutex = cxa_mutex_reserve());
		cxa_logger_init(&sysLog, "sysLog");
	}
}


static void writeField(const char *const stringIn, size_t maxFieldLenIn)
{
	size_t stringLen_bytes = strlen(stringIn);

	if( stringLen_bytes > maxFieldLenIn )
	{
		cxa_ioStream_writeBytes(ioStream, (void*)stringIn, maxFieldLenIn-strlen(CXA_LOGGER_TRUNCATE_STRING));
		cxa_ioStream_writeString(ioStream, CXA_LOGGER_TRUNCATE_STRING);
	}
	else
	{
		cxa_ioStream_writeString(ioStream, (char*)stringIn);
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
	const char *levelText = "UNKN";
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

	// our buffer for this go-round max...our pointer size
	// plus [0x] plus null-term
	char buff[sizeof(loggerIn)*2 + 4 + 1];

	// print the time (if enabled)
	#ifdef CXA_LOGGER_TIME_ENABLE
		snprintf(buff, sizeof(buff), "%-8" PRIx32, cxa_timeBase_getCount_us());
		// 32-bit integer +space
		writeField(buff, 9);
	#endif


	// print the name
	writeField(loggerIn->name, largestloggerName_bytes);

	// pointer (id of logger)
	snprintf(buff, sizeof(buff), "[%p]", loggerIn);
	writeField(buff, 5+(2*sizeof(void*)));

	// level text
	writeField(levelText, 5);
	cxa_ioStream_writeByte(ioStream, ' ');
}
