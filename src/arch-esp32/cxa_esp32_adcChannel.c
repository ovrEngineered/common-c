/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_esp32_adcChannel.h"


// ******** includes ********
#include <cxa_assert.h>
#include "esp_adc_cal.h"
#include <string.h>


// ******** local macro definitions ********
#define DEFAULT_VREF    				1100
#define ATTENUATION						ADC_ATTEN_DB_11
#define BIT_WIDTH						ADC_WIDTH_BIT_12


// ******** local type definitions ********


// ******** local function prototypes ********
static void cxa_esp32_adcChannel_init_common(cxa_esp32_adcChannel_t *const adcChanIn, adc_unit_t unitIn, adc1_channel_t chan1In, adc2_channel_t chan2In);
static void scm_startConversion_singleShot(cxa_adcChannel_t *const superIn);
static uint16_t scm_getMaxRawValue(cxa_adcChannel_t *const superIn);


// ********  local variable declarations *********
static bool isAdcPoweredOn = false;

static bool isAdc1Calibrated = false;
static esp_adc_cal_characteristics_t adc_chars_unit1;
static bool isAdc2Calibrated = false;
static esp_adc_cal_characteristics_t adc_chars_unit2;


// ******** global function implementations ********
void cxa_esp32_adcChannel_init_unit1(cxa_esp32_adcChannel_t *const adcChanIn, adc1_channel_t chanIn)
{
	cxa_assert(adcChanIn);

	cxa_esp32_adcChannel_init_common(adcChanIn, ADC_UNIT_1, chanIn, 0);
}


void cxa_esp32_adcChannel_init_unit2(cxa_esp32_adcChannel_t *const adcChanIn, adc2_channel_t chanIn)
{
	cxa_assert(adcChanIn);

	cxa_esp32_adcChannel_init_common(adcChanIn, ADC_UNIT_2, 0, chanIn);
}


// ******** local function implementations ********
static void cxa_esp32_adcChannel_init_common(cxa_esp32_adcChannel_t *const adcChanIn, adc_unit_t unitIn, adc1_channel_t chan1In, adc2_channel_t chan2In)
{
	cxa_assert(adcChanIn);

	// save our references
	adcChanIn->adcUnit = unitIn;
	adcChanIn->chan1 = chan1In;
	adcChanIn->chan1 = chan2In;

	// make sure the ADC is powered on
	if( !isAdcPoweredOn )
	{
		adc_power_acquire();
		isAdcPoweredOn = true;
	}

	// 12-bit, attenuate for the full voltage range
	adc1_config_width(BIT_WIDTH);
	adc1_config_channel_atten(adcChanIn->chan1, ATTENUATION);

	// load calibration values
	if( (adcChanIn->adcUnit == ADC_UNIT_1) && !isAdc1Calibrated )
	{
		memset(&adc_chars_unit1, 0, sizeof(adc_chars_unit1));
		esp_adc_cal_characterize(ADC_UNIT_1, ATTENUATION, BIT_WIDTH, DEFAULT_VREF, &adc_chars_unit1);
		isAdc1Calibrated = true;
	}
	if( (adcChanIn->adcUnit == ADC_UNIT_2) && !isAdc2Calibrated )
	{
		memset(&adc_chars_unit2, 0, sizeof(adc_chars_unit2));
		esp_adc_cal_characterize(ADC_UNIT_2, ATTENUATION, BIT_WIDTH, DEFAULT_VREF, &adc_chars_unit2);
		isAdc2Calibrated = true;
	}

	// initialize our superclass
	cxa_adcChannel_init(&adcChanIn->super,
						scm_startConversion_singleShot,
						scm_getMaxRawValue);
}



static void scm_startConversion_singleShot(cxa_adcChannel_t *const superIn)
{
	cxa_esp32_adcChannel_t* adcChanIn = (cxa_esp32_adcChannel_t*)superIn;
	cxa_assert(adcChanIn);

	int adc2_raw;
	uint16_t convVal_raw = 0;
	float voltage = 0.0;
	switch( adcChanIn->adcUnit )
	{
		case ADC_UNIT_1:
			convVal_raw = adc1_get_raw(adcChanIn->chan1);
			voltage = esp_adc_cal_raw_to_voltage(convVal_raw, &adc_chars_unit1) / 1000.0;
			break;

		case ADC_UNIT_2:
			adc2_get_raw(adcChanIn->chan2, BIT_WIDTH, &adc2_raw);
			convVal_raw = adc2_raw;
			voltage = esp_adc_cal_raw_to_voltage(convVal_raw, &adc_chars_unit2) / 1000.0;
			break;

		default:
			cxa_adcChannel_notify_conversionComplete(&adcChanIn->super, false, 0.0, 0);
			break;
	}

	// notify our listeners
	cxa_adcChannel_notify_conversionComplete(&adcChanIn->super, true, voltage, convVal_raw);
}


static uint16_t scm_getMaxRawValue(cxa_adcChannel_t *const superIn)
{
	cxa_esp32_adcChannel_t* adcChanIn = (cxa_esp32_adcChannel_t*)superIn;
	cxa_assert(adcChanIn);

	return 4095;
}
