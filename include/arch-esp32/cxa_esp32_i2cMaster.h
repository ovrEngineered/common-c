/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_ESP32_I2C_MASTER_H_
#define CXA_ESP32_I2C_MASTER_H_


// ******** includes ********
#include <cxa_i2cMaster.h>

#include "driver/i2c.h"


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_esp32_i2cMaster_t object
 */
typedef struct cxa_esp32_i2cMaster cxa_esp32_i2cMaster_t;


/**
 * @private
 */
struct cxa_esp32_i2cMaster
{
	cxa_i2cMaster_t super;

	i2c_port_t i2cPort;

	i2c_config_t conf;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_esp32_i2cMaster_init(cxa_esp32_i2cMaster_t *const i2cIn, const i2c_port_t i2cPortIn,
							 const gpio_num_t pinNum_sdaIn, const gpio_num_t pinNum_sclIn,
							 const bool enablePullUpsIn, uint32_t busFreq_hzIn);


#endif // CXA_BLUEGIGA_I2C_H_
