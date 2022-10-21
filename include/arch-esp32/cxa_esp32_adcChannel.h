/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_ESP32_ADCCHANNEL_H_
#define CXA_ESP32_ADCCHANNEL_H_


// ******** includes ********
#include <cxa_adcChannel.h>
#include "driver/adc.h"


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_esp32_adcChannel cxa_esp32_adcChannel_t;


/**
 * @private
 */
struct cxa_esp32_adcChannel
{
	cxa_adcChannel_t super;

	adc_unit_t adcUnit;

	adc1_channel_t chan1;
	adc2_channel_t chan2;
};


// ******** global function prototypes ********
void cxa_esp32_adcChannel_init_unit1(cxa_esp32_adcChannel_t *const adcChanIn, adc1_channel_t chanIn);
void cxa_esp32_adcChannel_init_unit2(cxa_esp32_adcChannel_t *const adcChanIn, adc2_channel_t chanIn);


#endif
