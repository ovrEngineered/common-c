/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_SILABSBGAPI_MODULE_H_
#define CXA_SILABSBGAPI_MODULE_H_


// ******** includes ********
#include <cxa_config.h>
#include <cxa_siLabsBgApi_btle_central.h>
#include <cxa_siLabsBgApi_btle_peripheral.h>
#include <cxa_ioStream.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef void (*cxa_siLabsBgApi_cb_onTimer_t)(void *const userVarIn);


// ******** global function prototypes ********
#ifndef CXA_SILABSBGAPI_MODE_SOC

/**
 * @public
 * Used to configure a SiliconLabs BGAPI module in a NCP configuration
 */
void cxa_siLabsBgApi_module_init(cxa_ioStream_t *const ioStreamIn, int threadIdIn);

#else

/**
 * @public
 * Used to configure a SiliconLabs BGAPI module in a SOC configuration
 */
void cxa_siLabsBgApi_module_init(void);
#endif


/**
 * @public
 */
cxa_siLabsBgApi_btle_central_t* cxa_siLabsBgApi_module_getBtleCentral(void);


/**
 * @public
 */
cxa_siLabsBgApi_btle_peripheral_t* cxa_siLabsBgApi_module_getBtlePeripheral(void);


/**
 * @public
 */
void cxa_siLabsBgApi_module_startSoftTimer_repeat(float period_msIn, cxa_siLabsBgApi_cb_onTimer_t cbIn, void* userVarIn);


/**
 * @public
 */
void cxa_siLabsBgApi_module_stopSoftTimer(cxa_siLabsBgApi_cb_onTimer_t cbIn, void* userVarIn);


/**
 * @protected
 */
cxa_btle_central_state_t cxa_siLabsBgApi_module_getState(void);


#endif
