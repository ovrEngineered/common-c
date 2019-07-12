/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_BLUEGIGA_GPIO_H_
#define CXA_BLUEGIGA_GPIO_H_


// ******** includes ********
#include <stdint.h>

#include <cxa_gpio.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * Forward declaration of blueGiga_btle_client to avoid
 * circular reference
 */
typedef struct cxa_blueGiga_btle_client cxa_blueGiga_btle_client_t;


/**
 * @public
 */
typedef void (*cxa_blueGiga_gpio_cb_onGpiosConfigured_t)(cxa_blueGiga_btle_client_t* btlecIn, bool wasSuccessfulIn);


/**
 * @private
 */
typedef struct
{
	cxa_gpio_t super;

	bool isUsed;

	cxa_blueGiga_btle_client_t* btlec;
	uint8_t portNum;
	uint8_t chanNum;

	cxa_gpio_polarity_t polarity;
	cxa_gpio_direction_t lastDirection;
	bool lastOutputVal;
}cxa_blueGiga_gpio_t;


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_blueGiga_gpio_init(cxa_blueGiga_gpio_t *const gpioIn, cxa_blueGiga_btle_client_t* btlecIn, uint8_t portNumIn, uint8_t chanNumIn);


/**
 * @protected
 */
void cxa_blueGiga_configureGpiosForBlueGiga(cxa_blueGiga_btle_client_t* btlecIn, cxa_blueGiga_gpio_cb_onGpiosConfigured_t cbIn);

#endif
