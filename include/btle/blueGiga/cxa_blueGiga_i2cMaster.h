/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_BLUEGIGA_I2C_MASTER_H_
#define CXA_BLUEGIGA_I2C_MASTER_H_


// ******** includes ********
#include <cxa_i2cMaster.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_blueGiga_i2cMaster_t object
 */
typedef struct cxa_blueGiga_i2cMaster cxa_blueGiga_i2cMaster_t;


/**
 * @public
 * Forward declaration of blueGiga_btle_client to avoid
 * circular reference
 */
typedef struct cxa_blueGiga_btle_client cxa_blueGiga_btle_client_t;


/**
 * @private
 */
struct cxa_blueGiga_i2cMaster
{
	cxa_i2cMaster_t super;

	cxa_blueGiga_btle_client_t* btlec;

	size_t expectedNumBytesToWrite;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_blueGiga_i2cMaster_init(cxa_blueGiga_i2cMaster_t *const i2cIn, cxa_blueGiga_btle_client_t* btlecIn);


#endif // CXA_BLUEGIGA_I2C_H_
