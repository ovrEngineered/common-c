/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_PCA9555_H_
#define CXA_PCA9555_H_


// ******** includes ********
#include <cxa_i2cMaster.h>
#include <cxa_gpio.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_pca9555 cxa_pca9555_t;


/**
 * @private
 */
typedef struct
{
	cxa_gpio_t super;

	cxa_pca9555_t* pca;
	uint8_t chanNum;

	cxa_gpio_polarity_t polarity;
	cxa_gpio_direction_t lastDirection;
	bool lastOutputVal;
}cxa_gpio_pca9555_t;


/**
 * @private
 */
struct cxa_pca9555
{
	cxa_i2cMaster_t* i2c;
	uint8_t address;

	cxa_gpio_pca9555_t gpios_port0[8];
	cxa_gpio_pca9555_t gpios_port1[8];
};


// ******** global function prototypes ********
bool cxa_pca9555_init(cxa_pca9555_t *const pcaIn, cxa_i2cMaster_t *const i2cIn, uint8_t addressIn);

cxa_gpio_t* cxa_pca9555_getGpio(cxa_pca9555_t *const pcaIn, uint8_t portNumIn, uint8_t chanNumIn);

#endif // CXA_PCA9555_H_
