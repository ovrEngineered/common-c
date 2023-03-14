/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_NUMBERUTILS_H_
#define CXA_NUMBERUTILS_H_


/**
 * @file
 * This is a file which contains utility functions for manipulating numbers
 *
 * @author Christopher Armenio
 */


// ******** includes ********
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// ******** global macro definitions ********
#define CXA_MAX(x, y) 								(((x) > (y)) ? (x) : (y))
#define CXA_MIN(x, y) 								(((x) < (y)) ? (x) : (y))

#define CXA_CLAMP_HIGH(val, max)					((val) = CXA_MIN((val), (max)))
#define CXA_CLAMP_LOW(val, min)						((val) = CXA_MAX((val), (min)))
#define CXA_CLAMP_LOW_HIGH(val, min, max)			((val) = CXA_CLAMP_HIGH(CXA_CLAMP_LOW((val), (min)), (max)))

#define CXA_INCREMENT_CLAMP_HIGH(val, inc, max)		((val) = (((max) - (val)) >= (inc)) ? ((val) + (inc)) : (max))
#define CXA_INCREMENT_CLAMP_LOW(val, inc, min)		((val) = (((min) + (val)) >= (fabs(inc))) ? ((val) + (inc)) : (min))


// ******** global type definitions *********


// ******** global function prototypes ********
uint16_t cxa_numberUtils_crc16_oneShot(void* dataIn, size_t dataLen_bytesIn);

uint16_t cxa_numberUtils_crc16_step(uint16_t crcIn, uint8_t byteIn);


#endif // CXA_NUMBERUTILS_H_
