/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_SNTPCLIENT_H_
#define CXA_SNTPCLIENT_H_


// ******** includes ********
#include <stdbool.h>
#include <stdint.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef void (*cxa_sntpClient_cb_onInitialTimeSet_t)(void *const userVarIn);


// ******** global function prototypes ********
void cxa_sntpClient_init(void);
void cxa_sntpClient_addListener(cxa_sntpClient_cb_onInitialTimeSet_t cbIn, void *const userVarIn);
bool cxa_sntpClient_isClockSet(void);

uint32_t cxa_sntpClient_getUnixTimeStamp(void);

#endif
