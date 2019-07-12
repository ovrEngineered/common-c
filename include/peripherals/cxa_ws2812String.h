/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_WS2812STRING_H_
#define CXA_WS2812STRING_H_


// ******** includes ********
#include <stddef.h>
#include <stdint.h>
#include <cxa_stateMachine.h>
#include <cxa_timeDiff.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
}cxa_ws2812String_pixelBuffer_t;


/**
 * @public
 */
typedef struct cxa_ws2812String cxa_ws2812String_t;


/**
 * @protected
 */
typedef void (*cxa_ws2812String_scm_writeBytes_t)(cxa_ws2812String_t *const superIn, cxa_ws2812String_pixelBuffer_t* pixelBuffersIn, size_t numPixelBuffersIn);


/**
 * @private
 */
struct cxa_ws2812String
{
	cxa_ws2812String_pixelBuffer_t* pixelBuffers;
	size_t numPixels;

	cxa_ws2812String_scm_writeBytes_t scm_writeBytes;

	cxa_stateMachine_t stateMachine;
	cxa_timeDiff_t td_fade;
	uint32_t lastUpdateTime_ms;

	struct
	{
		cxa_ws2812String_pixelBuffer_t targetColor;
		uint32_t fadeInPeriod_ms;
		uint32_t fadeOutPeriod_ms;

		float rVal;
	} pulseColor;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_ws2812String_init(cxa_ws2812String_t *const ws2812In,
						   cxa_ws2812String_pixelBuffer_t* pixelBuffersIn, size_t numPixelBuffersIn,
						   cxa_ws2812String_scm_writeBytes_t scm_writeBytesIn,
						   int threadIdIn);


/**
 * @public
 */
void cxa_ws2812String_blank_now(cxa_ws2812String_t *const ws2812In);


/**
 * @public
 */
void cxa_ws2812String_setPixel_rgb(cxa_ws2812String_t *const ws2812In, size_t pixelNumIn, uint8_t rIn, uint8_t gIn, uint8_t bIn);


/**
 * @public
 */
void cxa_ws2812String_setPixel_hsv(cxa_ws2812String_t *const ws2812In, size_t pixelNumIn, uint8_t hIn, uint8_t sIn, uint8_t vIn);


/**
 * @public
 */
void cxa_ws2812String_pulseColor_rgb(cxa_ws2812String_t *const ws2812In, uint32_t pulsePeriod_msIn, uint8_t rIn, uint8_t gIn, uint8_t bIn);


#endif /* CXA_WS2812STRING_H_ */
