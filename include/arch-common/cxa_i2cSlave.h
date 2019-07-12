/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_I2C_SLAVE_H_
#define CXA_I2C_SLAVE_H_


// ******** includes ********
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include <cxa_array.h>
#include <cxa_config.h>
#include <cxa_fixedByteBuffer.h>
#include <cxa_logger_header.h>
#include <cxa_timeDiff.h>


// ******** global macro definitions ********
#ifndef CXA_I2CSLAVE_MAXNUM_COMMANDS
#define CXA_I2CSLAVE_MAXNUM_COMMANDS			4
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_i2cSlave cxa_i2cSlave_t;


/**
 * @public
 */
typedef bool (*cxa_i2cSlave_cb_commandRead_t)(cxa_fixedByteBuffer_t *const bytesOut, void *const userVarIn);


/**
 * @public
 */
typedef void (*cxa_i2cSlave_cb_commandWrite_t)(cxa_fixedByteBuffer_t *const bytesIn, void *const userVarIn);


/**
 * @private
 */
typedef struct
{
	uint8_t command;

	cxa_i2cSlave_cb_commandRead_t cb_read;
	cxa_i2cSlave_cb_commandWrite_t cb_write;

	ssize_t expectedNumBytes;

	void *userVar;
}cxa_i2cSlave_commandEntry_t;


/**
 * @private
 */
struct cxa_i2cSlave
{
	cxa_array_t commands;
	cxa_i2cSlave_commandEntry_t commands_raw[CXA_I2CSLAVE_MAXNUM_COMMANDS];

	cxa_logger_t logger;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_i2cSlave_init(cxa_i2cSlave_t *const i2cIn);


/**
 * @public
 */
void cxa_i2cSlave_addCommand(cxa_i2cSlave_t *const i2cIn, const uint8_t commandIn,
							 cxa_i2cSlave_cb_commandRead_t cb_readIn,
							 cxa_i2cSlave_cb_commandWrite_t cb_writeIn,
							 const ssize_t expectedNumBytesIn,
							 void *const userVarIn);

/**
 * protected
 */
ssize_t cxa_i2cSlave_validatePreWrite(cxa_i2cSlave_t *const i2cIn, const uint8_t commandIn);


/**
 * @protected
 */
bool cxa_i2cSlave_notifyRead(cxa_i2cSlave_t *const i2cIn, const uint8_t commandIn, cxa_fixedByteBuffer_t *const bytesOut);


/**
 * @protected
 */
void cxa_i2cSlave_notifyWrite(cxa_i2cSlave_t *const i2cIn, const uint8_t commandIn, cxa_fixedByteBuffer_t *const bytesIn);


#endif // CXA_I2C_SLAVE_H_
