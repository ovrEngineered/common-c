/*
 * Copyright 2019 ovrEngineered, LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining 
 * a copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
 * copies of the Software, and to permit persons to whom the Software is furnished to 
 * do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all 
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
 * @protected
 */
cxa_btle_central_state_t cxa_siLabsBgApi_module_getState(void);


#endif
