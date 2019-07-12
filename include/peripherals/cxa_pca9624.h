/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_PCA9624_H_
#define CXA_PCA9624_H_


// ******** includes ********
#include <cxa_array.h>
#include <cxa_gpio.h>
#include <cxa_i2cMaster.h>
#include <cxa_logger_header.h>
#include <cxa_stateMachine.h>


// ******** global macro definitions ********
#define CXA_PCA9624_NUM_CHANNELS				8

#ifndef CXA_PCA9624_MAXNUM_LISTENERS
	#define CXA_PCA9624_MAXNUM_LISTENERS		2
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_pca9624 cxa_pca9624_t;


/**
 * @public
 */
typedef void (*cxa_pca9624_cb_t)(void *const userVarIn);


/**
 * @public
 */
typedef struct
{
	uint8_t channelIndex;
	uint8_t brightness;
}cxa_pca9624_channelEntry_t;


/**
 * @private
 */
typedef struct
{
	cxa_pca9624_cb_t cb_onBecomesReady;
	cxa_pca9624_cb_t cb_onError;

	void* userVar;
}cxa_pca9624_listener_t;


/**
 * @private
 */
struct cxa_pca9624
{
	cxa_i2cMaster_t* i2c;
	uint8_t address;

	cxa_gpio_t* gpio_outputEnable;

	uint8_t currRegs[18];

	cxa_logger_t logger;
	cxa_stateMachine_t stateMachine;

	cxa_array_t listeners;
	cxa_pca9624_listener_t listeners_raw[CXA_PCA9624_MAXNUM_LISTENERS];
};


// ******** global function prototypes ********
void cxa_pca9624_init(cxa_pca9624_t *const pcaIn, cxa_i2cMaster_t *const i2cIn, uint8_t addressIn, cxa_gpio_t *const gpio_oeIn, int threadIdIn);

void cxa_pca9624_addListener(cxa_pca9624_t *const pcaIn, cxa_pca9624_cb_t cb_onBecomesReadyIn, cxa_pca9624_cb_t cb_onErrorIn, void *userVarIn);

void cxa_pca9624_start(cxa_pca9624_t *const pcaIn);

bool cxa_pca9624_hasStartedSuccessfully(cxa_pca9624_t *const pcaIn);

void cxa_pca9624_setGlobalBlinkRate(cxa_pca9624_t *const pcaIn, uint16_t onPeriod_msIn, uint16_t offPeriod_msIn);

void cxa_pca9624_setBrightnessForChannels(cxa_pca9624_t *const pcaIn, cxa_pca9624_channelEntry_t* chansEntriesIn, size_t numChansIn);
void cxa_pca9624_blinkChannels(cxa_pca9624_t *const pcaIn, cxa_pca9624_channelEntry_t* chansEntriesIn, size_t numChansIn);

#endif // CXA_PCA9624_H_
