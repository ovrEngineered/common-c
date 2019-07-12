/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_UNIQUE_ID_H_
#define CXA_UNIQUE_ID_H_


// ******** includes ********
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
void cxa_uniqueId_getBytes(uint8_t** bytesOut, size_t* numBytesOut);
char* cxa_uniqueId_getHexString(void);


#endif // CXA_UNIQUE_ID_H_
