/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_FD_LINEPARSER_H_
#define CXA_FD_LINEPARSER_H_


// ******** includes ********
#include <stdio.h>
#include <stdbool.h>
#include <cxa_array.h>


// ******** global macro definitions ********


// ******** global type definitions *********
typedef void (*cxa_fdLineParser_lineCb_t)(uint8_t *lineStartIn, size_t numBytesInLineIn, void *userVarIn);

typedef struct
{
	FILE *fd;
	bool echoUser;

	cxa_fdLineParser_lineCb_t cb;
	void *userVar;

	bool wasLastByteCr;
	cxa_array_t lineBuffer;
}cxa_fdLineParser_t;


// ******** global function prototypes ********
void cxa_fdLineParser_init(cxa_fdLineParser_t *const fdlpIn, FILE *fdIn, bool echoUserIn, void *bufferIn, size_t bufferSize_bytesIn, cxa_fdLineParser_lineCb_t cbIn, void *userVarIn);

bool cxa_fdLineParser_update(cxa_fdLineParser_t *const fdlpIn);


#endif // CXA_FD_LINEPARSER_H_
