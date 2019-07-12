/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_RANDOM_H_
#define CXA_RANDOM_H_


// ******** includes ********
#include <stdint.h>


// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
/**
 * @public
 * @brief Generates a *random* number between the provided
 * lower and upper limit (inclusive)
 *
 * Random numbers here are provided to the best of the ability
 * of the platform / architecture and may not be truly random.
 *
 * @param[in] lowerLimitIn lower limit of the output
 * @param[in] uperLimitIn upper limit of the output
 */
uint32_t cxa_random_numberInRange(uint32_t lowerLimitIn, uint32_t upperLimitIn);


#endif
