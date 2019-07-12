/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_ATMEGA_I2CSLAVE_H_
#define CXA_ATMEGA_I2CSLAVE_H_


// ******** includes ********
#include <stdint.h>

#include <cxa_fixedFifo.h>
#include <cxa_i2cSlave.h>


// ******** global macro definitions ********
#ifndef CXA_ATM_I2CSLAVE_MAXWRITE_SIZE_BYTES
#define CXA_ATM_I2CSLAVE_MAXOP_SIZE_BYTES		16
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_atmega_i2cSlave cxa_atmega_i2cSlave_t;


/**
 * @public
 */
typedef enum
{
	CXA_ATM_I2CSLAVE_TWI0,
	CXA_ATM_I2CSLAVE_TWI1
}cxa_atmega_i2cSlave_twi_t;


/**
 * @private
 */
typedef enum
{
	CXA_ATM_I2CSLAVE_STATE_IDLE,
	CXA_ATM_I2CSLAVE_STATE_WAIT_COMMAND_BYTE,
	CXA_ATM_I2CSLAVE_STATE_GOT_COMMAND_BYTE,
	CXA_ATM_I2CSLAVE_STATE_RX_BYTES,
	CXA_ATM_I2CSLAVE_STATE_TX_BYTES,
	CXA_ATM_I2CSLAVE_STATE_NACK_REST,
}cxa_atmega_i2cSlave_state_t;


/**
 * @private
 */
struct cxa_atmega_i2cSlave
{
	cxa_i2cSlave_t super;

	cxa_atmega_i2cSlave_twi_t twiIn;

	cxa_atmega_i2cSlave_state_t state;
	uint8_t currCommandByte;
	uint8_t currTxByteIndex;
	ssize_t expectedNumRxBytes;

	cxa_fixedByteBuffer_t buffer;
	uint8_t buffer_raw[CXA_ATM_I2CSLAVE_MAXOP_SIZE_BYTES];
};


// ******** global function prototypes ********
void cxa_atmega_i2cSlave_init(cxa_atmega_i2cSlave_t *const i2cIn,
							  const cxa_atmega_i2cSlave_twi_t twiIn,
							  uint8_t address_7bitIn);


#endif
