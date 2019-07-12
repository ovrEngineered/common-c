/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_TEMPSENSOR_ADC_H_
#define CXA_TEMPSENSOR_ADC_H_


// ******** includes ********
#include <cxa_adcChannel.h>
#include <cxa_tempSensor.h>


// ******** global macro definitions ********
#define CXA_CELSIUS_TO_KELVIN_OFFSET			273.15


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_tempSensor_adc cxa_tempSensor_adc_t;


/**
 * private
 */
typedef enum
{
	CXA_TEMPSENSOR_CALTYPE_ONEPOINTVOLTAGE,
	CXA_TEMPSENSOR_CALTYPE_BETA,
}cxa_tempSensor_calibrationType_t;


/**
 * @private
 */
struct cxa_tempSensor_adc
{
	cxa_tempSensor_t super;

	cxa_adcChannel_t* adc;

	cxa_tempSensor_calibrationType_t calibrationType;

	union
	{
		struct
		{
			float knownTemp_c;
			float vAtKnownTemp;
		} onePointCal;

		struct
		{
			float r1_ohm;
			float r0_ohm;
			float t0_c;
			float beta;
		} beta;
	} calibrationVals;
};



// ******** global function prototypes ********
void cxa_tempSensor_adc_init_voltageOnePoint(cxa_tempSensor_adc_t *const tempSnsIn, cxa_adcChannel_t *const adcChanIn,
							 	 	 	 	 float knownTemp_cIn, float vAtKnownTempIn);

/**
 * @public
 * Initializes temp sensor using "Beta Parameter" calibration
 *
 * @param tempSnsIn pointer to a pre-allocated tempSensor object
 * @param adcChanIn pointer to a pre-initialized ADC channel used
 * 					to measure the temperature
 * @param r1_ohmIn	resistance of the top resistor of the voltage divider
 * 					(thermistor is the bottom resistor)
 * @param r0_ohmIn	resistance of the thermistor at reference temperature t0
 * @param t0_cIn	reference temperature (usually 25C)
 * @param betaIn	beta value of the thermistor
 */
void cxa_tempSensor_adc_init_beta(cxa_tempSensor_adc_t *const tempSnsIn, cxa_adcChannel_t *const adcChanIn,
								  float r1_ohmIn, float r0_ohmIn, float t0_cIn, float betaIn);


#endif /* CXA_TEMPSENSOR_ADC_H_ */
