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


// ******** global macro definitions ********
#define CXA_LOG_LEVEL_NONE				0
#define CXA_LOG_LEVEL_ERROR				1
#define CXA_LOG_LEVEL_WARN				2
#define CXA_LOG_LEVEL_INFO				3
#define CXA_LOG_LEVEL_DEBUG				4
#define CXA_LOG_LEVEL_TRACE				5

#if( (CXA_LOG_LEVEL == CXA_LOG_LEVEL_NONE) || (!defined CXA_LOG_LEVEL) )
	#define cxa_logger_error(loggerIn, msgIn, ...)
	#define cxa_logger_warn(loggerIn, msgIn, ...)
	#define cxa_logger_info(loggerIn, msgIn, ...)
	#define cxa_logger_debug(loggerIn, msgIn, ...)
	#define cxa_logger_trace(loggerIn, msgIn, ...)
#elif( CXA_LOG_LEVEL == CXA_LOG_LEVEL_ERROR )
	#define cxa_logger_error(loggerIn, msgIn, ...)		cxa_logger_vlog((loggerIn), CXA_LOG_LEVEL_ERROR, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_warn(loggerIn, msgIn, ...)
	#define cxa_logger_info(loggerIn, msgIn, ...)
	#define cxa_logger_debug(loggerIn, msgIn, ...)
	#define cxa_logger_trace(loggerIn, msgIn, ...)
#elif( CXA_LOG_LEVEL == CXA_LOG_LEVEL_WARN )
	#define cxa_logger_error(loggerIn, msgIn, ...)		cxa_logger_vlog((loggerIn), CXA_LOG_LEVEL_ERROR, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_warn(loggerIn, msgIn, ...)		cxa_logger_vlog((loggerIn), CXA_LOG_LEVEL_WARN, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_info(loggerIn, msgIn, ...)
	#define cxa_logger_debug(loggerIn, msgIn, ...)
	#define cxa_logger_trace(loggerIn, msgIn, ...)
#elif( CXA_LOG_LEVEL == CXA_LOG_LEVEL_INFO )
	#define cxa_logger_error(loggerIn, msgIn, ...)		cxa_logger_vlog((loggerIn), CXA_LOG_LEVEL_ERROR, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_warn(loggerIn, msgIn, ...)		cxa_logger_vlog((loggerIn), CXA_LOG_LEVEL_WARN, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_info(loggerIn, msgIn, ...)		cxa_logger_vlog((loggerIn), CXA_LOG_LEVEL_INFO, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_debug(loggerIn, msgIn, ...)
	#define cxa_logger_trace(loggerIn, msgIn, ...)
#elif( CXA_LOG_LEVEL == CXA_LOG_LEVEL_DEBUG )
	#define cxa_logger_error(loggerIn, msgIn, ...)		cxa_logger_vlog((loggerIn), CXA_LOG_LEVEL_ERROR, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_warn(loggerIn, msgIn, ...)		cxa_logger_vlog((loggerIn), CXA_LOG_LEVEL_WARN, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_info(loggerIn, msgIn, ...)		cxa_logger_vlog((loggerIn), CXA_LOG_LEVEL_INFO, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_debug(loggerIn, msgIn, ...)		cxa_logger_vlog((loggerIn), CXA_LOG_LEVEL_DEBUG, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_trace(loggerIn, msgIn, ...)
#elif( CXA_LOG_LEVEL == CXA_LOG_LEVEL_TRACE )
	#define cxa_logger_error(loggerIn, msgIn, ...)		cxa_logger_vlog((loggerIn), CXA_LOG_LEVEL_ERROR, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_warn(loggerIn, msgIn, ...)		cxa_logger_vlog((loggerIn), CXA_LOG_LEVEL_WARN, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_info(loggerIn, msgIn, ...)		cxa_logger_vlog((loggerIn), CXA_LOG_LEVEL_INFO, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_debug(loggerIn, msgIn, ...)		cxa_logger_vlog((loggerIn), CXA_LOG_LEVEL_DEBUG, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_trace(loggerIn, msgIn, ...)		cxa_logger_vlog((loggerIn), CXA_LOG_LEVEL_TRACE, (msgIn), ##__VA_ARGS__)
#else
	#error "Unknown CXA_LOG_LEVEL specified"
#endif


// ******** global type definitions *********


// ******** global function prototypes ********
void cxa_logger_setGlobalFd(FILE *fd_outputIn);

void cxa_logger_init(cxa_logger_t *const loggerIn, const char *nameIn);
void cxa_logger_vinit(cxa_logger_t *const loggerIn, const char *nameFmtIn, ...);

void cxa_logger_vlog(cxa_logger_t *const loggerIn, const uint8_t levelIn, const char *formatIn, ...);


#endif // CXA_LOGGER_H_
