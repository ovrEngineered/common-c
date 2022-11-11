/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_stepperMotorChannel.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_array.h>
#include <cxa_console.h>
#include <cxa_delay.h>
#include <cxa_nvsManager.h>
#include <cxa_timeDiff.h>
#include <float.h>
#include <math.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define MAX_NUM_STEPPERS						11
#define ENTER_SLEEP_TIMEOUT_MS					5000

#define NVS_KEY_STEP_FREQ_HZ					"stpCtrl_sf_hz"
#define DEFAULT_STEP_FREQ_HZ					500



// ******** local type definitions ********


// ******** local function prototypes ********
static void consoleCb_getFrequency(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn);
static void consoleCb_setFrequency(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn);
static void consoleCb_getCurrentSteps(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn);
static void consoleCb_setTargetSteps(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn);


static void periodic_timer_callback(void);


// ********  local variable declarations *********
static bool isInit = false;
static cxa_logger_t logger;

static cxa_gpio_t* gpio_globalDir;
static cxa_gpio_t* gpio_globalEnable;
static cxa_timeDiff_t td_enterSleep;

static uint32_t stepFrequency_hz = DEFAULT_STEP_FREQ_HZ;

static cxa_array_t stepperCtrlrs;
static cxa_stepperMotorChannel_t* stepperCtrlrs_raw[MAX_NUM_STEPPERS];

static bool forceEnable = false;


// ******** global function implementations ********
void cxa_stepperMotorChannel_initSubsystem(cxa_gpio_t *const gpioGlobalDirIn, cxa_gpio_t *const gpioGlobalEnableIn)
{
	cxa_assert(!isInit);
	cxa_assert(gpioGlobalDirIn);
	cxa_assert(gpioGlobalEnableIn);

	// save our references
	gpio_globalDir = gpioGlobalDirIn;
	gpio_globalEnable = gpioGlobalEnableIn;

	// setup our subsystem logger
	cxa_logger_init(&logger, "stepCtrl-subSys");

	// setup our array to keep track of controllers
	cxa_array_initStd(&stepperCtrlrs, stepperCtrlrs_raw);

	// setup our sleep timeout
	cxa_timeDiff_init(&td_enterSleep);

	// try to load our step frequency from NVS
	cxa_nvsManager_get_uint32_withDefault(NVS_KEY_STEP_FREQ_HZ, &stepFrequency_hz, DEFAULT_STEP_FREQ_HZ);

	// register our console callbacks
	cxa_console_addCommand("stepCtrl_getFreq", "get step frequency", NULL, 0, consoleCb_getFrequency, NULL);
	cxa_console_argDescriptor_t args_setFreq[] = {
			{CXA_STRINGUTILS_DATATYPE_INTEGER, "freq_hz"}
	};
	cxa_console_addCommand("stepCtrl_setFreq", "set step frequency", args_setFreq, sizeof(args_setFreq)/sizeof(*args_setFreq), consoleCb_setFrequency, NULL);
	cxa_console_argDescriptor_t args_getCurrSteps[] = {
			{CXA_STRINGUTILS_DATATYPE_INTEGER, "chanNum"}
	};
	cxa_console_addCommand("stepCtrl_getSteps", "get current step count", args_getCurrSteps, sizeof(args_getCurrSteps)/sizeof(*args_getCurrSteps), consoleCb_getCurrentSteps, NULL);
	cxa_console_argDescriptor_t args_setTargetSteps[] = {
			{CXA_STRINGUTILS_DATATYPE_INTEGER, "chanNum"},
			{CXA_STRINGUTILS_DATATYPE_INTEGER, "targetSteps"}
	};
	cxa_console_addCommand("stepCtrl_setSteps", "set target step count", args_setTargetSteps, sizeof(args_setTargetSteps)/sizeof(*args_setTargetSteps), consoleCb_setTargetSteps, NULL);

	// mark our initialization as complete
	isInit = true;

	// schedule our timer
	cxa_stepperMotorChannel_registerTimedCallback(periodic_timer_callback, stepFrequency_hz);
}


void cxa_stepperMotorChannel_init(cxa_stepperMotorChannel_t *const stepMtrChanIn, cxa_gpio_t *const gpio_stepIn)
{
	cxa_assert(isInit);
	cxa_assert(stepMtrChanIn);
	cxa_assert(gpio_stepIn);

	// save our references
	stepMtrChanIn->gpio_step = gpio_stepIn;

	// set a good initial state
	cxa_gpio_setValue(stepMtrChanIn->gpio_step, 0);

	// add to our list of stepper controllers
	cxa_array_append(&stepperCtrlrs, (void *const)&stepMtrChanIn);
}


void cxa_stepperMotorChannel_incrementTargetPosition(cxa_stepperMotorChannel_t *const stepMtrChanIn, float incrementStepsIn)
{
	cxa_assert(stepMtrChanIn);
	cxa_assert(isInit);

	stepMtrChanIn->targetStep += incrementStepsIn;
}


bool cxa_stepperMotorChannel_isStopped(cxa_stepperMotorChannel_t *const stepMtrChanIn)
{
	cxa_assert(stepMtrChanIn);

	float diff_steps = stepMtrChanIn->targetStep - ((float)stepMtrChanIn->currentStep);
	return fabs(diff_steps) < 1.0 ;
}


void cxa_stepperMotorChannel_runContinuous(cxa_stepperMotorChannel_t *const stepMtrChanIn, bool forwardIn)
{
	cxa_assert(stepMtrChanIn);

	stepMtrChanIn->targetStep = forwardIn ? INFINITY : -INFINITY;
}


void cxa_stepperMotorChannel_stopMotion(cxa_stepperMotorChannel_t *const stepMtrChanIn)
{
	cxa_assert(stepMtrChanIn);

	stepMtrChanIn->targetStep = stepMtrChanIn->currentStep;
}


void cxa_stepperMotorChannel_forceEnable(bool forceEnableIn)
{
	forceEnable = forceEnableIn;
	if( forceEnable ) cxa_gpio_setValue(gpio_globalEnable, 1);
}


// ******** local function implementations ********
static void consoleCb_getFrequency(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn)
{
	cxa_ioStream_writeFormattedLine(ioStreamIn, "stepFrequency: %lu Hz", stepFrequency_hz);
}


static void consoleCb_setFrequency(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn)
{
	cxa_stringUtils_parseResult_t* stepFreq_hzIn = cxa_array_get(argsIn, 0);
	stepFrequency_hz = stepFreq_hzIn->val_int;

	// reschedule our timer
	cxa_stepperMotorChannel_registerTimedCallback(periodic_timer_callback, stepFrequency_hz);

	// save our value to NVS (after we reschedule so we can catch out-of-range values)
	cxa_nvsManager_set_uint32(NVS_KEY_STEP_FREQ_HZ, stepFrequency_hz);
	cxa_nvsManager_commit();
}


static void consoleCb_getCurrentSteps(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn)
{
	cxa_stringUtils_parseResult_t* channelNumIn = cxa_array_get(argsIn, 0);

	cxa_ioStream_writeFormattedLine(ioStreamIn, "currSteps: %lu", stepperCtrlrs_raw[channelNumIn->val_int]->currentStep);
//	cxa_ioStream_writeFormattedLine(ioStreamIn, "%.2f, %.2f", stepperCtrlrs_raw[channelNumIn->val_int]->targetStep, ((float)(stepperCtrlrs_raw[channelNumIn->val_int]->currentStep)));
}


static void consoleCb_setTargetSteps(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn)
{
	cxa_stringUtils_parseResult_t* channelNumIn = cxa_array_get(argsIn, 0);
	cxa_stringUtils_parseResult_t* targetStepsIn = cxa_array_get(argsIn, 1);

	stepperCtrlrs_raw[channelNumIn->val_int]->targetStep = targetStepsIn->val_int;
}


// ******** interrupt implementations ********
static void periodic_timer_callback(void)
{
	if( !isInit ) return;

	// send all of the positive-going pulses we need
	bool didPerformStep = false;
	cxa_array_iterate(&stepperCtrlrs, currStepCtrl, cxa_stepperMotorChannel_t*)
	{
		if( currStepCtrl == NULL ) continue;

		float diff_steps = (*currStepCtrl)->targetStep - ((float)(*currStepCtrl)->currentStep);
		if( fabs(diff_steps) >= 1.0 )
		{
			// check to see if we are asleep...if so, wake us up
			if( !forceEnable && !cxa_gpio_getValue(gpio_globalEnable) )
			{
				cxa_logger_debug(&logger, "waking up");
				cxa_gpio_setValue(gpio_globalEnable, 1);
			}

			// set our GPIO direction as needed
			cxa_gpio_setValue(gpio_globalDir, (diff_steps > 0));

			// perform the rising edge
			cxa_gpio_setValue((*currStepCtrl)->gpio_step, 1);
			(*currStepCtrl)->currentStep = (*currStepCtrl)->currentStep +
					((((*currStepCtrl)->currentStep < (*currStepCtrl)->targetStep)) ? 1 : -1);

			didPerformStep = true;
		}
	}
	if( !didPerformStep )
	{
		// no steps to perform...check our enter sleep timer
		if( !forceEnable && cxa_gpio_getValue(gpio_globalEnable) )
		{
			// we're not asleep yet...check if we need to be
			if( cxa_timeDiff_isElapsed_ms(&td_enterSleep, ENTER_SLEEP_TIMEOUT_MS) )
			{
				// go to sleep
				cxa_logger_debug(&logger, "sleeping");
				cxa_gpio_setValue(gpio_globalEnable, 0);
			}
		}
		return;
	}
	// if we made it here, we're actually moving motors

	// reset our enter sleep timer
	cxa_timeDiff_setStartTime_now(&td_enterSleep);

	// wait for hold time
	cxa_delay_us(2);

	// now return to 0
	cxa_array_iterate(&stepperCtrlrs, currStepCtrl, cxa_stepperMotorChannel_t*)
	{
		if( currStepCtrl == NULL ) continue;
		cxa_gpio_setValue((*currStepCtrl)->gpio_step, 0);
	}
}
