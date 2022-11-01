/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_LOGGER_IMPL_H_
#define CXA_LOGGER_IMPL_H_


// ******** includes ********
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <cxa_logger_header.h>
#include <cxa_ioStream.h>


// ******** global macro definitions ********
#define CXA_LOG_LEVEL_NONE				0
#define CXA_LOG_LEVEL_ERROR				1
#define CXA_LOG_LEVEL_WARN				2
#define CXA_LOG_LEVEL_INFO				3
#define CXA_LOG_LEVEL_DEBUG				4
#define CXA_LOG_LEVEL_TRACE				5

#ifdef CXA_LOGGER_CLAMPED_ENABLE
#define _cxa_logger_clamped_wrapper(loggerIn, levelIn, period_msIn, msgIn, ...) 							\
	if( cxa_timeDiff_isElapsed_ms(&((loggerIn)->td_clamped), (period_msIn)) ) {								\
		cxa_logger_log_formattedString_impl((loggerIn), (levelIn), (msgIn), ##__VA_ARGS__);					\
		cxa_timeDiff_setStartTime_now(&((loggerIn)->td_clamped));											\
	}
#endif


#if( (defined CXA_LOGGER_DISABLE) || !(defined CXA_LOG_LEVEL) || (CXA_LOG_LEVEL == CXA_LOG_LEVEL_NONE) )
	#define cxa_logger_error(loggerIn, msgIn, ...)
	#define cxa_logger_warn(loggerIn, msgIn, ...)
	#define cxa_logger_info(loggerIn, msgIn, ...)
	#define cxa_logger_debug(loggerIn, msgIn, ...)
	#define cxa_logger_trace(loggerIn, msgIn, ...)

	#define cxa_logger_error_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)
	#define cxa_logger_warn_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)
	#define cxa_logger_info_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)
	#define cxa_logger_debug_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)
	#define cxa_logger_trace_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)

	#define cxa_logger_error_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)
	#define cxa_logger_warn_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)
	#define cxa_logger_info_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)
	#define cxa_logger_debug_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)
	#define cxa_logger_trace_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)

	#define cxa_logger_error_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)
	#define cxa_logger_warn_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)
	#define cxa_logger_info_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)
	#define cxa_logger_debug_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)
	#define cxa_logger_trace_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)

	#ifdef CXA_LOGGER_CLAMPED_ENABLE
	#define cxa_logger_clamped_error(loggerIn, period_msIn, msgIn, ...)
	#define cxa_logger_clamped_warn(loggerIn, period_msIn, msgIn, ...)
	#define cxa_logger_clamped_info(loggerIn, period_msIn, msgIn, ...)
	#define cxa_logger_clamped_debug(loggerIn, period_msIn, msgIn, ...)
	#define cxa_logger_clamped_trace(loggerIn, period_msIn, msgIn, ...)
	#endif

#elif( (defined CXA_LOG_LEVEL) && (CXA_LOG_LEVEL == CXA_LOG_LEVEL_ERROR) )
	#define cxa_logger_error(loggerIn, msgIn, ...)																	cxa_logger_log_formattedString_impl((loggerIn), CXA_LOG_LEVEL_ERROR, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_warn(loggerIn, msgIn, ...)
	#define cxa_logger_info(loggerIn, msgIn, ...)
	#define cxa_logger_debug(loggerIn, msgIn, ...)
	#define cxa_logger_trace(loggerIn, msgIn, ...)

	#define cxa_logger_error_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)		cxa_logger_log_untermString_impl(loggerIn, CXA_LOG_LEVEL_ERROR, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)
	#define cxa_logger_warn_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)
	#define cxa_logger_info_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)
	#define cxa_logger_debug_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)
	#define cxa_logger_trace_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)

	#define cxa_logger_error_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)							cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_ERROR, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)
	#define cxa_logger_warn_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)
	#define cxa_logger_info_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)
	#define cxa_logger_debug_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)
	#define cxa_logger_trace_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)

	#define cxa_logger_error_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)										cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_ERROR, prefixIn, cxa_fixedByteBuffer_get_pointerToStartOfData((fbbIn)), cxa_fixedByteBuffer_getSize_bytes((fbbIn)), postFixIn)
	#define cxa_logger_warn_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)
	#define cxa_logger_info_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)
	#define cxa_logger_debug_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)
	#define cxa_logger_trace_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)

	#ifdef CXA_LOGGER_CLAMPED_ENABLE
	#define cxa_logger_clamped_error(loggerIn, period_msIn, msgIn, ...)												_cxa_logger_clamped_wrapper((loggerIn), CXA_LOG_LEVEL_ERROR, (period_msIn), (msgIn), ##__VA_ARGS__)
	#define cxa_logger_clamped_warn(loggerIn, period_msIn, msgIn, ...)
	#define cxa_logger_clamped_info(loggerIn, period_msIn, msgIn, ...)
	#define cxa_logger_clamped_debug(loggerIn, period_msIn, msgIn, ...)
	#define cxa_logger_clamped_trace(loggerIn, period_msIn, msgIn, ...)
	#endif

#elif( (defined CXA_LOG_LEVEL) && (CXA_LOG_LEVEL == CXA_LOG_LEVEL_WARN) )
	#define cxa_logger_error(loggerIn, msgIn, ...)		cxa_logger_log_formattedString_impl((loggerIn), CXA_LOG_LEVEL_ERROR, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_warn(loggerIn, msgIn, ...)		cxa_logger_log_formattedString_impl((loggerIn), CXA_LOG_LEVEL_WARN, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_info(loggerIn, msgIn, ...)
	#define cxa_logger_debug(loggerIn, msgIn, ...)
	#define cxa_logger_trace(loggerIn, msgIn, ...)

	#define cxa_logger_error_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)		cxa_logger_log_untermString_impl(loggerIn, CXA_LOG_LEVEL_ERROR, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)
	#define cxa_logger_warn_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)		cxa_logger_log_untermString_impl(loggerIn, CXA_LOG_LEVEL_WARN, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)
	#define cxa_logger_info_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)
	#define cxa_logger_debug_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)
	#define cxa_logger_trace_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)

	#define cxa_logger_error_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)							cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_ERROR, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)
	#define cxa_logger_warn_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)							cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_WARN, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)
	#define cxa_logger_info_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)
	#define cxa_logger_debug_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)
	#define cxa_logger_trace_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)

	#define cxa_logger_error_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)										cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_ERROR, prefixIn, cxa_fixedByteBuffer_get_pointerToStartOfData((fbbIn)), cxa_fixedByteBuffer_getSize_bytes((fbbIn)), postFixIn)
	#define cxa_logger_warn_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)										cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_WARN, prefixIn, cxa_fixedByteBuffer_get_pointerToStartOfData((fbbIn)), cxa_fixedByteBuffer_getSize_bytes((fbbIn)), postFixIn)
	#define cxa_logger_info_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)
	#define cxa_logger_debug_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)
	#define cxa_logger_trace_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)

	#ifdef CXA_LOGGER_CLAMPED_ENABLE
	#define cxa_logger_clamped_error(loggerIn, period_msIn, msgIn, ...)												_cxa_logger_clamped_wrapper((loggerIn), CXA_LOG_LEVEL_ERROR, (period_msIn), (msgIn), ##__VA_ARGS__)
	#define cxa_logger_clamped_warn(loggerIn, period_msIn, msgIn, ...)												_cxa_logger_clamped_wrapper((loggerIn), CXA_LOG_LEVEL_WARN, (period_msIn), (msgIn), ##__VA_ARGS__)
	#define cxa_logger_clamped_info(loggerIn, period_msIn, msgIn, ...)
	#define cxa_logger_clamped_debug(loggerIn, period_msIn, msgIn, ...)
	#define cxa_logger_clamped_trace(loggerIn, period_msIn, msgIn, ...)
	#endif

#elif( (defined CXA_LOG_LEVEL) && (CXA_LOG_LEVEL == CXA_LOG_LEVEL_INFO) )
	#define cxa_logger_error(loggerIn, msgIn, ...)		cxa_logger_log_formattedString_impl((loggerIn), CXA_LOG_LEVEL_ERROR, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_warn(loggerIn, msgIn, ...)		cxa_logger_log_formattedString_impl((loggerIn), CXA_LOG_LEVEL_WARN, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_info(loggerIn, msgIn, ...)		cxa_logger_log_formattedString_impl((loggerIn), CXA_LOG_LEVEL_INFO, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_debug(loggerIn, msgIn, ...)
	#define cxa_logger_trace(loggerIn, msgIn, ...)

	#define cxa_logger_error_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)		cxa_logger_log_untermString_impl(loggerIn, CXA_LOG_LEVEL_ERROR, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)
	#define cxa_logger_warn_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)		cxa_logger_log_untermString_impl(loggerIn, CXA_LOG_LEVEL_WARN, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)
	#define cxa_logger_info_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)		cxa_logger_log_untermString_impl(loggerIn, CXA_LOG_LEVEL_INFO, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)
	#define cxa_logger_debug_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)
	#define cxa_logger_trace_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)

	#define cxa_logger_error_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)							cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_ERROR, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)
	#define cxa_logger_warn_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)							cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_WARN, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)
	#define cxa_logger_info_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)							cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_INFO, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)
	#define cxa_logger_debug_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)
	#define cxa_logger_trace_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)

	#define cxa_logger_error_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)										cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_ERROR, prefixIn, cxa_fixedByteBuffer_get_pointerToStartOfData((fbbIn)), cxa_fixedByteBuffer_getSize_bytes((fbbIn)), postFixIn)
	#define cxa_logger_warn_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)										cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_WARN, prefixIn, cxa_fixedByteBuffer_get_pointerToStartOfData((fbbIn)), cxa_fixedByteBuffer_getSize_bytes((fbbIn)), postFixIn)
	#define cxa_logger_info_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)										cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_INFO, prefixIn, cxa_fixedByteBuffer_get_pointerToStartOfData((fbbIn)), cxa_fixedByteBuffer_getSize_bytes((fbbIn)), postFixIn)
	#define cxa_logger_debug_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)
	#define cxa_logger_trace_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)

	#ifdef CXA_LOGGER_CLAMPED_ENABLE
	#define cxa_logger_clamped_error(loggerIn, period_msIn, msgIn, ...)												_cxa_logger_clamped_wrapper((loggerIn), CXA_LOG_LEVEL_ERROR, (period_msIn), (msgIn), ##__VA_ARGS__)
	#define cxa_logger_clamped_warn(loggerIn, period_msIn, msgIn, ...)												_cxa_logger_clamped_wrapper((loggerIn), CXA_LOG_LEVEL_WARN, (period_msIn), (msgIn), ##__VA_ARGS__)
	#define cxa_logger_clamped_info(loggerIn, period_msIn, msgIn, ...)												_cxa_logger_clamped_wrapper((loggerIn), CXA_LOG_LEVEL_INFO, (period_msIn), (msgIn), ##__VA_ARGS__)
	#define cxa_logger_clamped_debug(loggerIn, period_msIn, msgIn, ...)
	#define cxa_logger_clamped_trace(loggerIn, period_msIn, msgIn, ...)
	#endif

#elif( (defined CXA_LOG_LEVEL) && (CXA_LOG_LEVEL == CXA_LOG_LEVEL_DEBUG) )
	#define cxa_logger_error(loggerIn, msgIn, ...)		cxa_logger_log_formattedString_impl((loggerIn), CXA_LOG_LEVEL_ERROR, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_warn(loggerIn, msgIn, ...)		cxa_logger_log_formattedString_impl((loggerIn), CXA_LOG_LEVEL_WARN, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_info(loggerIn, msgIn, ...)		cxa_logger_log_formattedString_impl((loggerIn), CXA_LOG_LEVEL_INFO, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_debug(loggerIn, msgIn, ...)		cxa_logger_log_formattedString_impl((loggerIn), CXA_LOG_LEVEL_DEBUG, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_trace(loggerIn, msgIn, ...)

	#define cxa_logger_error_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)		cxa_logger_log_untermString_impl(loggerIn, CXA_LOG_LEVEL_ERROR, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)
	#define cxa_logger_warn_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)		cxa_logger_log_untermString_impl(loggerIn, CXA_LOG_LEVEL_WARN, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)
	#define cxa_logger_info_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)		cxa_logger_log_untermString_impl(loggerIn, CXA_LOG_LEVEL_INFO, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)
	#define cxa_logger_debug_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)		cxa_logger_log_untermString_impl(loggerIn, CXA_LOG_LEVEL_DEBUG, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)
	#define cxa_logger_trace_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)

	#define cxa_logger_error_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)							cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_ERROR, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)
	#define cxa_logger_warn_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)							cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_WARN, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)
	#define cxa_logger_info_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)							cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_INFO, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)
	#define cxa_logger_debug_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)							cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_DEBUG, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)
	#define cxa_logger_trace_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)

	#define cxa_logger_error_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)										cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_ERROR, prefixIn, cxa_fixedByteBuffer_get_pointerToStartOfData((fbbIn)), cxa_fixedByteBuffer_getSize_bytes((fbbIn)), postFixIn)
	#define cxa_logger_warn_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)										cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_WARN, prefixIn, cxa_fixedByteBuffer_get_pointerToStartOfData((fbbIn)), cxa_fixedByteBuffer_getSize_bytes((fbbIn)), postFixIn)
	#define cxa_logger_info_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)										cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_INFO, prefixIn, cxa_fixedByteBuffer_get_pointerToStartOfData((fbbIn)), cxa_fixedByteBuffer_getSize_bytes((fbbIn)), postFixIn)
	#define cxa_logger_debug_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)										cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_DEBUG, prefixIn, cxa_fixedByteBuffer_get_pointerToStartOfData((fbbIn)), cxa_fixedByteBuffer_getSize_bytes((fbbIn)), postFixIn)
	#define cxa_logger_trace_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)

	#ifdef CXA_LOGGER_CLAMPED_ENABLE
	#define cxa_logger_clamped_error(loggerIn, period_msIn, msgIn, ...)												_cxa_logger_clamped_wrapper((loggerIn), CXA_LOG_LEVEL_ERROR, (period_msIn), (msgIn), ##__VA_ARGS__)
	#define cxa_logger_clamped_warn(loggerIn, period_msIn, msgIn, ...)												_cxa_logger_clamped_wrapper((loggerIn), CXA_LOG_LEVEL_WARN, (period_msIn), (msgIn), ##__VA_ARGS__)
	#define cxa_logger_clamped_info(loggerIn, period_msIn, msgIn, ...)												_cxa_logger_clamped_wrapper((loggerIn), CXA_LOG_LEVEL_INFO, (period_msIn), (msgIn), ##__VA_ARGS__)
	#define cxa_logger_clamped_debug(loggerIn, period_msIn, msgIn, ...)												_cxa_logger_clamped_wrapper((loggerIn), CXA_LOG_LEVEL_DEBUG, (period_msIn), (msgIn), ##__VA_ARGS__)
	#define cxa_logger_clamped_trace(loggerIn, period_msIn, msgIn, ...)
	#endif

#elif( (defined CXA_LOG_LEVEL) && (CXA_LOG_LEVEL == CXA_LOG_LEVEL_TRACE) )
	#define cxa_logger_error(loggerIn, msgIn, ...)		cxa_logger_log_formattedString_impl((loggerIn), CXA_LOG_LEVEL_ERROR, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_warn(loggerIn, msgIn, ...)		cxa_logger_log_formattedString_impl((loggerIn), CXA_LOG_LEVEL_WARN, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_info(loggerIn, msgIn, ...)		cxa_logger_log_formattedString_impl((loggerIn), CXA_LOG_LEVEL_INFO, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_debug(loggerIn, msgIn, ...)		cxa_logger_log_formattedString_impl((loggerIn), CXA_LOG_LEVEL_DEBUG, (msgIn), ##__VA_ARGS__)
	#define cxa_logger_trace(loggerIn, msgIn, ...)		cxa_logger_log_formattedString_impl((loggerIn), CXA_LOG_LEVEL_TRACE, (msgIn), ##__VA_ARGS__)

	#define cxa_logger_error_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)		cxa_logger_log_untermString_impl(loggerIn, CXA_LOG_LEVEL_ERROR, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)
	#define cxa_logger_warn_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)		cxa_logger_log_untermString_impl(loggerIn, CXA_LOG_LEVEL_WARN, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)
	#define cxa_logger_info_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)		cxa_logger_log_untermString_impl(loggerIn, CXA_LOG_LEVEL_INFO, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)
	#define cxa_logger_debug_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)		cxa_logger_log_untermString_impl(loggerIn, CXA_LOG_LEVEL_DEBUG, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)
	#define cxa_logger_trace_untermString(loggerIn, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)		cxa_logger_log_untermString_impl(loggerIn, CXA_LOG_LEVEL_TRACE, prefixIn, untermStringIn, untermStrLen_bytesIn, postFixIn)

	#define cxa_logger_error_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)							cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_ERROR, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)
	#define cxa_logger_warn_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)							cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_WARN, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)
	#define cxa_logger_info_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)							cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_INFO, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)
	#define cxa_logger_debug_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)							cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_DEBUG, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)
	#define cxa_logger_trace_memDump(loggerIn, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)							cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_TRACE, prefixIn, ptrIn, ptrLen_bytesIn, postFixIn)

	#define cxa_logger_error_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)										cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_ERROR, prefixIn, cxa_fixedByteBuffer_get_pointerToStartOfData((fbbIn)), cxa_fixedByteBuffer_getSize_bytes((fbbIn)), postFixIn)
	#define cxa_logger_warn_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)										cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_WARN, prefixIn, cxa_fixedByteBuffer_get_pointerToStartOfData((fbbIn)), cxa_fixedByteBuffer_getSize_bytes((fbbIn)), postFixIn)
	#define cxa_logger_info_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)										cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_INFO, prefixIn, cxa_fixedByteBuffer_get_pointerToStartOfData((fbbIn)), cxa_fixedByteBuffer_getSize_bytes((fbbIn)), postFixIn)
	#define cxa_logger_debug_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)										cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_DEBUG, prefixIn, cxa_fixedByteBuffer_get_pointerToStartOfData((fbbIn)), cxa_fixedByteBuffer_getSize_bytes((fbbIn)), postFixIn)
	#define cxa_logger_trace_memDump_fbb(loggerIn, prefixIn, fbbIn, postFixIn)										cxa_logger_log_memdump_impl(loggerIn, CXA_LOG_LEVEL_TRACE, prefixIn, cxa_fixedByteBuffer_get_pointerToStartOfData((fbbIn)), cxa_fixedByteBuffer_getSize_bytes((fbbIn)), postFixIn)

	#ifdef CXA_LOGGER_CLAMPED_ENABLE
	#define cxa_logger_clamped_error(loggerIn, period_msIn, msgIn, ...)												_cxa_logger_clamped_wrapper((loggerIn), CXA_LOG_LEVEL_ERROR, (period_msIn), (msgIn), ##__VA_ARGS__)
	#define cxa_logger_clamped_warn(loggerIn, period_msIn, msgIn, ...)												_cxa_logger_clamped_wrapper((loggerIn), CXA_LOG_LEVEL_WARN, (period_msIn), (msgIn), ##__VA_ARGS__)
	#define cxa_logger_clamped_info(loggerIn, period_msIn, msgIn, ...)												_cxa_logger_clamped_wrapper((loggerIn), CXA_LOG_LEVEL_INFO, (period_msIn), (msgIn), ##__VA_ARGS__)
	#define cxa_logger_clamped_debug(loggerIn, period_msIn, msgIn, ...)												_cxa_logger_clamped_wrapper((loggerIn), CXA_LOG_LEVEL_DEBUG, (period_msIn), (msgIn), ##__VA_ARGS__)
	#define cxa_logger_clamped_trace(loggerIn, period_msIn, msgIn, ...)												_cxa_logger_clamped_wrapper((loggerIn), CXA_LOG_LEVEL_TRACE, (period_msIn), (msgIn), ##__VA_ARGS__)
	#endif

#else
	#error "Unknown CXA_LOG_LEVEL specified"
#endif

#define cxa_logger_stepDebug()								cxa_logger_stepDebug_formattedString_impl(__FILE__, __LINE__, NULL)
#define cxa_logger_stepDebug_msg(msgIn, ...)				cxa_logger_stepDebug_formattedString_impl(__FILE__, __LINE__, (msgIn), ##__VA_ARGS__)
#define cxa_logger_stepDebug_memDump(msgIn, ptrIn, lenIn)	cxa_logger_stepDebug_memDump_impl(__FILE__, __LINE__, (ptrIn), (lenIn), (msgIn))
#define cxa_logger_stepDebug_memDump_fbb(msgIn, fbbIn)		cxa_logger_stepDebug_memDump_impl(__FILE__, __LINE__, cxa_fixedByteBuffer_get_pointerToIndex((fbbIn),0), cxa_fixedByteBuffer_getSize_bytes((fbbIn)), (msgIn))


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
 * @private
 */
void cxa_logger_log_formattedString_impl(cxa_logger_t *const loggerIn, const uint8_t levelIn, const char* formatIn, ...);


/**
 * @private
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
void cxa_logger_log_untermString_impl(cxa_logger_t *const loggerIn, const uint8_t levelIn, const char* prefixIn, const char* untermStringIn, size_t untermStrLen_bytesIn, const char* postFixIn);


/**
 * @private
 */
void cxa_logger_log_memdump_impl(cxa_logger_t *const loggerIn, const uint8_t levelIn, const char* prefixIn, const void* ptrIn, size_t ptrLen_bytes, const char* postFixIn);


/**
 * @private
 */
void cxa_logger_stepDebug_formattedString_impl(const char* fileIn, const int lineNumIn, const char* formatIn, ...);


/**
 * @private
 */
void cxa_logger_stepDebug_memDump_impl(const char* fileIn, const int lineNumIn, void* bytesIn, size_t numBytesIn, const char* msgIn);


#endif // CXA_LOGGER_H_
