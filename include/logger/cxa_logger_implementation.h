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
#ifndef CXA_LOGGER_IMPL_H_
#define CXA_LOGGER_IMPL_H_


/**
 * @file <description>
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdio.h>
#include <stdint.h>
#include <cxa_logger_header.h>
#include <cxa_ioStream.h>


// ******** global macro definitions ********
#define CXA_LOG_LEVEL_NONE				0
#define CXA_LOG_LEVEL_ERROR				1
#define CXA_LOG_LEVEL_WARN				2
#define CXA_LOG_LEVEL_INFO				3
#define CXA_LOG_LEVEL_DEBUG				4
#define CXA_LOG_LEVEL_TRACE				5

#if( (!defined CXA_LOG_LEVEL) || (CXA_LOG_LEVEL == CXA_LOG_LEVEL_NONE) )
	#define cxa_logger_error(loggerIn, msgIn, ...)
	#define cxa_logger_warn(loggerIn, msgIn, ...)
	#define cxa_logger_info(loggerIn, msgIn, ...)
	#define cxa_logger_debug(loggerIn, msgIn, ...)
	#define cxa_logger_trace(loggerIn, msgIn, ...)
#elif( (defined CXA_LOG_LEVEL) && (CXA_LOG_LEVEL == CXA_LOG_LEVEL_ERROR) )
	#define cxa_logger_error(loggerIn, msgIn, ...)		cxa_logger_log_formattedString((loggerIn), CXA_LOG_LEVEL_ERROR, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_warn(loggerIn, msgIn, ...)
	#define cxa_logger_info(loggerIn, msgIn, ...)
	#define cxa_logger_debug(loggerIn, msgIn, ...)
	#define cxa_logger_trace(loggerIn, msgIn, ...)
#elif( (defined CXA_LOG_LEVEL) && (CXA_LOG_LEVEL == CXA_LOG_LEVEL_WARN) )
	#define cxa_logger_error(loggerIn, msgIn, ...)		cxa_logger_log_formattedString((loggerIn), CXA_LOG_LEVEL_ERROR, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_warn(loggerIn, msgIn, ...)		cxa_logger_log_formattedString((loggerIn), CXA_LOG_LEVEL_WARN, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_info(loggerIn, msgIn, ...)
	#define cxa_logger_debug(loggerIn, msgIn, ...)
	#define cxa_logger_trace(loggerIn, msgIn, ...)
#elif( (defined CXA_LOG_LEVEL) && (CXA_LOG_LEVEL == CXA_LOG_LEVEL_INFO) )
	#define cxa_logger_error(loggerIn, msgIn, ...)		cxa_logger_log_formattedString((loggerIn), CXA_LOG_LEVEL_ERROR, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_warn(loggerIn, msgIn, ...)		cxa_logger_log_formattedString((loggerIn), CXA_LOG_LEVEL_WARN, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_info(loggerIn, msgIn, ...)		cxa_logger_log_formattedString((loggerIn), CXA_LOG_LEVEL_INFO, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_debug(loggerIn, msgIn, ...)
	#define cxa_logger_trace(loggerIn, msgIn, ...)
#elif( (defined CXA_LOG_LEVEL) && (CXA_LOG_LEVEL == CXA_LOG_LEVEL_DEBUG) )
	#define cxa_logger_error(loggerIn, msgIn, ...)		cxa_logger_log_formattedString((loggerIn), CXA_LOG_LEVEL_ERROR, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_warn(loggerIn, msgIn, ...)		cxa_logger_log_formattedString((loggerIn), CXA_LOG_LEVEL_WARN, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_info(loggerIn, msgIn, ...)		cxa_logger_log_formattedString((loggerIn), CXA_LOG_LEVEL_INFO, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_debug(loggerIn, msgIn, ...)		cxa_logger_log_formattedString((loggerIn), CXA_LOG_LEVEL_DEBUG, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_trace(loggerIn, msgIn, ...)
#elif( (defined CXA_LOG_LEVEL) && (CXA_LOG_LEVEL == CXA_LOG_LEVEL_TRACE) )
	#define cxa_logger_error(loggerIn, msgIn, ...)		cxa_logger_log_formattedString((loggerIn), CXA_LOG_LEVEL_ERROR, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_warn(loggerIn, msgIn, ...)		cxa_logger_log_formattedString((loggerIn), CXA_LOG_LEVEL_WARN, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_info(loggerIn, msgIn, ...)		cxa_logger_log_formattedString((loggerIn), CXA_LOG_LEVEL_INFO, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_debug(loggerIn, msgIn, ...)		cxa_logger_log_formattedString((loggerIn), CXA_LOG_LEVEL_DEBUG, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_trace(loggerIn, msgIn, ...)		cxa_logger_log_formattedString((loggerIn), CXA_LOG_LEVEL_TRACE, (msgIn), ##__VA_ARGS__)
#else
	#error "Unknown CXA_LOG_LEVEL specified"
#endif

#define cxa_logger_stepDebug()							cxa_logger_stepDebug_formattedString(__FILE__, __LINE__, NULL)
#define cxa_logger_stepDebug_msg(msgIn, ...)			cxa_logger_stepDebug_formattedString(__FILE__, __LINE__, (msgIn), ##__VA_ARGS__)


// ******** global type definitions *********


// ******** global function prototypes ********
/**
 * @public
 * @brief Sets the ioStream which will be used to output all
 * 		logging statements.
 * @param ioStreamIn the pre-initialized ioStream which will
 * 		be used to output logging statements
 */
void cxa_logger_setGlobalIoStream(cxa_ioStream_t *const ioStreamIn);

/**
 * @public
 * @brief Initializes a logger with the given name
 *
 * @param loggerIn pointer to the pre-allocated logger to
 * 		initialize
 * @param nameIn the logger's name (will be copied into the
 * 		logger object)
 */
void cxa_logger_init(cxa_logger_t *const loggerIn, const char *nameIn);

/**
 * @public
 * @brief Convenience method to initialize a logger with a format string
 * @param loggerIn pointer to the pre-allocated logger to
 * 		initialize
 * @param nameIn the logger's name format string (will be copied into the
 * 		logger object)
 */
void cxa_logger_init_formattedString(cxa_logger_t *const loggerIn, const char *nameFmtIn, ...);

/**
 * @public
 * @brief Returns the system logger. Should be used for debugging only
 * (eg. on systems that don't support native printf)
 *
 * @return The system logger
 */
cxa_logger_t* cxa_logger_getSysLog(void);

/**
 * @public
 * @brief Logs a string that is not null terminated (and thus the length
 * cannot be calculated)
 *
 * @param loggerIn the pre-initialized logger
 * @param levelIn the desired logging level
 * @param prefixIn a null-terminated string that should be printed before the
 * 		the unterminated string
 * @param untermStringIn pointer to the unterminated string
 * @param untermStrLen_bytesIn number of bytes of the unterminated string to print
 * @param postFixIn a null-terminated string that should be printed immediately
 * 		folowing the unterminated string
 */
void cxa_logger_log_untermString(cxa_logger_t *const loggerIn, const uint8_t levelIn, const char* prefixIn, const char* untermStringIn, size_t untermStrLen_bytesIn, const char* postFixIn);

/**
 * @private
 */
void cxa_logger_stepDebug_formattedString(const char* fileIn, const int lineNumIn, const char* formatIn, ...);

/**
 * @private
 */
void cxa_logger_log_formattedString(cxa_logger_t *const loggerIn, const uint8_t levelIn, const char *formatIn, ...);


#endif // CXA_LOGGER_H_
