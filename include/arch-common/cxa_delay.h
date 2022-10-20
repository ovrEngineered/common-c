/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */

/**
 * @file
 * This file contains functions for delaying continued execution by a fixed amount of time.
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * // start here @ 0sec
 * cxa_delay_ms(1000);
 * // reach here @ 1sec
 * @endcode
 */
#ifndef CXA_DELAY_H_
#define CXA_DELAY_H_


// ******** includes ********
#include <stdint.h>


// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
/**
 * @public
 * @brief Delays for a fixed number of milliseconds
 *
 * @param[in] delay_msIn the number of milliseconds for which to delay
 */
void cxa_delay_ms(uint16_t delay_msIn);


/**
 * @public
 * @brief Delays for a fixed number of microseconds
 *
 * @param[in] delay_usIn the number of microseconds for which to delay
 */
void cxa_delay_us(uint32_t delay_usIn);


#endif // CXA_DELAY_H_
