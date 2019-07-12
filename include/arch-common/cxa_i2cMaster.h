/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_I2C_MASTER_H_
#define CXA_I2C_MASTER_H_


// ******** includes ********
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <cxa_fixedByteBuffer.h>
#include <cxa_timeDiff.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_i2cMaster cxa_i2cMaster_t;


/**
 * @public
 */
typedef void (*cxa_i2cMaster_cb_onReadComplete_t)(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const readBytesIn, void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_i2cMaster_cb_onWriteComplete_t)(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, void* userVarIn);


/**
 * @private
 */
typedef void (*cxa_i2cMaster_scm_readBytes_t)(cxa_i2cMaster_t *const superIn,
											 uint8_t addressIn, uint8_t sendStopIn, size_t numBytesToReadIn);


/**
 * @private
 */
typedef void (*cxa_i2cMaster_scm_readBytesWithControlBytes_t)(cxa_i2cMaster_t *const superIn,
															 uint8_t addressIn, uint8_t sendStopIn,
															 cxa_fixedByteBuffer_t *const controlBytesIn,
															 size_t numBytesToReadIn);


/**
 * @private
 */
typedef void (*cxa_i2cMaster_scm_writeBytes_t)(cxa_i2cMaster_t *const superIn,
											   uint8_t addressIn, uint8_t sendStopIn, cxa_fixedByteBuffer_t *const writeBytesIn);


/**
 * @private
 */
typedef void (*cxa_i2cMaster_scm_resetBus_t)(cxa_i2cMaster_t *const superIn);


/**
 * @private
 */
struct cxa_i2cMaster
{
	bool isBusy;

	struct
	{
		cxa_i2cMaster_scm_readBytes_t readBytes;
		cxa_i2cMaster_scm_readBytesWithControlBytes_t readBytesWithControlBytes;
		cxa_i2cMaster_scm_writeBytes_t writeBytes;
		cxa_i2cMaster_scm_resetBus_t resetBus;
	}scms;

	struct
	{
		cxa_i2cMaster_cb_onReadComplete_t readComplete;
		cxa_i2cMaster_cb_onWriteComplete_t writeComplete;

		void* userVar;
	}cbs;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_i2cMaster_init(cxa_i2cMaster_t *const i2cIn,
					    cxa_i2cMaster_scm_readBytes_t scm_readIn,
						cxa_i2cMaster_scm_readBytesWithControlBytes_t scm_readWithControlBytesIn,
						cxa_i2cMaster_scm_writeBytes_t scm_writeIn,
						cxa_i2cMaster_scm_resetBus_t scm_resetBusIn);

/**
 * @public
 */
void cxa_i2cMaster_resetBus(cxa_i2cMaster_t *const i2cIn);

/**
 * @public
 */
void cxa_i2cMaster_readBytes(cxa_i2cMaster_t *const i2cIn,
							 uint8_t addressIn, uint8_t sendStopIn, size_t numBytesToReadIn,
							 cxa_i2cMaster_cb_onReadComplete_t cbIn, void* userVarIn);

/**
 * @public
 */
void cxa_i2cMaster_readBytes_withControlBytes(cxa_i2cMaster_t *const i2cIn,
											 uint8_t addressIn, uint8_t sendStopIn,
											 cxa_fixedByteBuffer_t *const controlBytesIn,
											 size_t numBytesToReadIn,
											 cxa_i2cMaster_cb_onReadComplete_t cbIn, void* userVarIn);


/**
 * @public
 */
void cxa_i2cMaster_writeBytes(cxa_i2cMaster_t *const i2cIn,
							  uint8_t addressIn, uint8_t sendStopIn, cxa_fixedByteBuffer_t *const writeBytesIn,
							  cxa_i2cMaster_cb_onWriteComplete_t cbIn, void* userVarIn);


/**
 * @protected
 */
void cxa_i2cMaster_notify_readComplete(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn, cxa_fixedByteBuffer_t *const readBytesIn);
void cxa_i2cMaster_notify_writeComplete(cxa_i2cMaster_t *const i2cIn, bool wasSuccessfulIn);

#endif // CXA_I2C_MASTER_H_
