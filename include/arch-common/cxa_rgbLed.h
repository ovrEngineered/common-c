/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_RGBLED_H_
#define CXA_RGBLED_H_


// ******** includes ********
#include <stdint.h>


// ******** global macro definitions ********
#define CXA_RGBLED_OFF				0x00, 0x00, 0x00
#define CXA_RGBLED_RED				0xFF, 0x00, 0x00
#define CXA_RGBLED_GREEN			0x00, 0xFF, 0x00
#define CXA_RGBLED_BLUE				0x00, 0x00, 0xFF
#define CXA_RGBLED_ORANGE			0xFF, 0x80, 0x00
#define CXA_RGBLED_CYAN				0x00, 0xFF, 0xFF



// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_rgbLed cxa_rgbLed_t;


/**
 * @public
 */
typedef enum
{
	CXA_RGBLED_STATE_SOLID,
	CXA_RGBLED_STATE_ALTERNATE_COLORS,
	CXA_RGBLED_STATE_FLASHONCE
}cxa_rgbLed_state_t;


/**
 * @protected
 */
typedef void (*cxa_rgbLed_scm_setRgb_t)(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn);


/**
 * @protected
 */
typedef void (*cxa_rgbLed_scm_alternateColors_t)(cxa_rgbLed_t *const superIn,
												uint8_t r1In, uint8_t g1In, uint8_t b1In,
												uint16_t color1Period_msIn,
												uint8_t r2In, uint8_t g2In, uint8_t b2In,
												uint16_t color2Period_msIn);

/**
 * @protected
 */
typedef void (*cxa_rgbLed_scm_flashOnce_t)(cxa_rgbLed_t *const superIn, uint8_t rIn, uint8_t gIn, uint8_t bIn,
									   	   uint16_t period_msIn);

/**
 * @private
 */
struct cxa_rgbLed
{
	cxa_rgbLed_state_t prevState;
	cxa_rgbLed_state_t currState;

	uint8_t maxBright_pcnt100;

	struct
	{
		cxa_rgbLed_scm_setRgb_t setRgb;
		cxa_rgbLed_scm_alternateColors_t alternateColors;
		cxa_rgbLed_scm_flashOnce_t flashOnce;
	}scms;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_rgbLed_init(cxa_rgbLed_t *const ledIn,
					 cxa_rgbLed_scm_setRgb_t scm_setRgbIn,
					 cxa_rgbLed_scm_alternateColors_t scm_alternateColorsIn,
					 cxa_rgbLed_scm_flashOnce_t scm_flashOnceIn);


/**
 * @public
 */
void cxa_rgbLed_setMaxBright_pcnt100(cxa_rgbLed_t *const ledIn, const uint8_t max_pcnt100In);


/**
 * @public
 */
void cxa_rgbLed_setRgb(cxa_rgbLed_t *const ledIn, uint8_t rIn, uint8_t gIn, uint8_t bIn);


/**
 * @public
 */
void cxa_rgbLed_blink(cxa_rgbLed_t *const ledIn, uint8_t rIn, uint8_t gIn, uint8_t bIn,
					  uint16_t onPeriod_msIn, uint16_t offPeriod_msIn);


/**
 * @public
 */
void cxa_rgbLed_alternateColors(cxa_rgbLed_t *const superIn,
							   uint8_t r1In, uint8_t g1In, uint8_t b1In,
							   uint16_t color1Period_msIn,
							   uint8_t r2In, uint8_t g2In, uint8_t b2In,
							   uint16_t color2Period_msIn);


/**
 * @public
 */
void cxa_rgbLed_flashOnce(cxa_rgbLed_t *const ledIn, uint8_t rIn, uint8_t gIn, uint8_t bIn,
						  uint16_t period_msIn);

#endif /* CXA_RGBLED_H_ */
