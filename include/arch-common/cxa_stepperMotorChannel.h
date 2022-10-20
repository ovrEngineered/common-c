/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_STEPPERMOTORCHANNEL_H_
#define CXA_STEPPERMOTORCHANNEL_H_


// ******** includes ********
#include <cxa_logger_header.h>
#include <cxa_gpio.h>
#include <stdint.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_stepperMotorChannel cxa_stepperMotorChannel_t;


/**
 * @private
 */
typedef void (*cxa_stepperMotorChannel_timedCallback_t)(void);


/**
 * @private
 */
struct cxa_stepperMotorChannel
{
	cxa_gpio_t* gpio_step;

	int32_t currentStep;
	float targetStep;
};


// ******** global function prototypes ********
/**
 * @public
 */
void cxa_stepperMotorChannel_initSubsystem(cxa_gpio_t *const gpioGlobalDirIn, cxa_gpio_t *const gpioGlobalEnableIn);


/**
 * @public
 */
void cxa_stepperMotorChannel_init(cxa_stepperMotorChannel_t *const stepMtrChanIn, cxa_gpio_t *const gpio_stepIn);


/**
 * @public
 */
void cxa_stepperMotorChannel_incrementTargetPosition(cxa_stepperMotorChannel_t *const stepMtrChanIn, float incrementStepsIn);


/**
 * @public
 */
bool cxa_stepperMotorChannel_isStopped(cxa_stepperMotorChannel_t *const stepMtrChanIn);


/**
 * @public
 */
void cxa_stepperMotorChannel_runContinuous(cxa_stepperMotorChannel_t *const stepMtrChanIn, bool forwardIn);


/**
 * @public
 */
void cxa_stepperMotorChannel_stopMotion(cxa_stepperMotorChannel_t *const stepMtrChanIn);


/**
 * @protected
 */
void cxa_stepperMotorChannel_registerTimedCallback(cxa_stepperMotorChannel_timedCallback_t cbIn, uint32_t stepFreq_hzIn);


#endif
