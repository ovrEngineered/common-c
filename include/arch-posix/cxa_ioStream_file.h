/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_IOSTREAM_FILE_H_
#define CXA_IOSTREAM_FILE_H_


// ******** includes ********
#include <stdbool.h>
#include <stdio.h>
#include <cxa_ioStream.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_ioStream_file_t object
 */
typedef struct cxa_ioStream_file cxa_ioStream_file_t;


struct cxa_ioStream_file
{
	cxa_ioStream_t super;

	FILE* file;
};


// ******** global function prototypes ********
void cxa_ioStream_file_init(cxa_ioStream_file_t *const ioStreamIn);
void cxa_ioStream_file_setFile(cxa_ioStream_file_t *const ioStreamIn, FILE *const fileIn);
void cxa_ioStream_file_close(cxa_ioStream_file_t *const ioStreamIn);

#endif // CXA_IOSTREAM_FILE_H_
