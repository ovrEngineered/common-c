/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
 
/**
 * @file
 * This file contains prototypes and a top-level implementation of a timeBase object.
 * A timeBase object is essentially a monotonically increasing counter which can be
 * used to calculate macro-scale, relative timeouts (on the order of seconds and minutes).
 *
 * In most implementations, the timeBase is backed by a monotonically increasing counter
 * of limited range (16 or 32 bit). In these cases, the counter will simply start counting
 * when initialized and continue to count up, until it overflows at ::cxa_timeBase_getMaxCount_us.
 * Due to this potential overflow, care must be taken to calculate timeouts while taking the
 * maximum value of the counter into account. It is not possible to judge elapsed time
 * beyond the range returned by ::cxa_timeBase_getMaxCount_us.
 *
 * @note This file contains the base functionality for a timeBase object available across all architectures. Additional
 *		functionality, including initialization is available in the architecture-specific implementation.
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * // initialization for architecture specific implementation
*
 * ...
 *
 * // get the current time in microseconds
 * uint32_t currTime_us = cxa_timeBase_getCount_us();
 * @endcode
 */
#ifndef CXA_TIMEBASE_H_
#define CXA_TIMEBASE_H_


// ******** includes ********
#include <stdint.h>


// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
/**
 * @public
 * @brief Returns the current monotonic, relative time in microseconds.
 *
 * @return the current time of the timeBase, in microseconds
 */
uint32_t cxa_timeBase_getCount_us(void);


/**
 * @public
 * @brief Returns the maximum value of the timeBase, in microseconds.
 * After reaching this value, the underlying timing mechanism will overflow to 0.
 * It is not possible to measure time periods longer than this value.
 *
 * @return the maximum time period measureable by this timeBase
 */
uint32_t cxa_timeBase_getMaxCount_us(void);


#endif // CXA_TIMEBASE_H_
