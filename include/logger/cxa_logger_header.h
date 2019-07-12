/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_LOGGER_HDR_H_
#define CXA_LOGGER_HDR_H_


// ******** includes ********
#include <cxa_config.h>


// ******** global macro definitions ********
#ifndef CXA_LOGGER_MAX_NAME_LEN_CHARS
	#define CXA_LOGGER_MAX_NAME_LEN_CHARS			24
#endif


// ******** global type definitions *********
typedef struct
{
	char name[CXA_LOGGER_MAX_NAME_LEN_CHARS+1];
}cxa_logger_t;


// ******** global function prototypes ********


#endif // CXA_LOGGER_HEADER_H_
