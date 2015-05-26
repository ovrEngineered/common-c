/**
 * @file
 * This file contains prototypes and an implementation of Timer Capture Channel for a
 * XMega 16-bit timer module. A capture channel is hardware peripheral specifically designed
 * to capture the value of a timer/count at a specific point in time (eg. a rising edge on a GPIO).
 *
 * On the XMega, all capture channels are triggered by an event channel. The event channel source
 * of the event channel can then be set to whatever event source you'd like. Some examples:
 *     * input pin to detect edges
 *     * timer overflow to create a 32-bit timer
 *     * ADC conversion complete to automatically timestamp an ADC reading
 *
 * @note This file contains functionality restricted to the CXA Atmel XMega implementation.
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * cxa_xmega_timer16_captureChannel_t myCaptureChannel;
 * // initializes the capture channel (PORTC Timer 0, Channel A) to be triggered by event channel 0
 * cxa_xmega_timer16_captureChannel_init(&myCaptureChannel, CXA_XMEGA_TIMER16_CAPTURE_CHANNEL_A, CXA_XMEGA_TIMER16_TCC0, CXA_XMEGA_EVENT_CHAN_0, false);
 *
 * // manually trigger event channel 0 just for show and read the value of the timer at that point in time
 * cxa_xmega_eventSystem_triggerEvents(CXA_XMEGA_EVENT_CHAN_0);
 * uint16_t value = uint16_t cxa_xmega_timer16_captureChannel_getLastCaptureVal(&myCaptureChannel);
 * @endcode
 *
 *
 * @copyright 2013-2014 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_XMEGA_TIMER16_CAPTURE_CHANNEL_H_
#define CXA_XMEGA_TIMER16_CAPTURE_CHANNEL_H_


// ******** includes ********
#include <stdint.h>
#include <stdbool.h>
#include <cxa_xmega_timer16.h>
#include <cxa_xmega_eventSystem.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_xmega_timer16_captureChannel_t object
 */
typedef struct cxa_xmega_timer16_captureChannel cxa_xmega_timer16_captureChannel_t;


/**
 * @public
 * @brief Enumeration containing all possible capture channels for a given timer.
 * @note some timers (PORT<X> Timer 1 specifically) only have two channels: A+B. If
 *		an attempt is made to configure channel C or D on these timers, an assert
 *		/ execution halt will occur.
 */
typedef enum
{
	CXA_XMEGA_TIMER16_CAPTURE_CHANNEL_A,									///< Channel A
	CXA_XMEGA_TIMER16_CAPTURE_CHANNEL_B,									///< Channel B
	CXA_XMEGA_TIMER16_CAPTURE_CHANNEL_C,									///< Channel C
	CXA_XMEGA_TIMER16_CAPTURE_CHANNEL_D,									///< Channel D
}cxa_xmega_timer16_captureChannel_enum_t;


/**
 * @private
 */
struct cxa_xmega_timer16_captureChannel
{
	cxa_xmega_timer16_captureChannel_enum_t enumChan;
	cxa_xmega_timer16_t timer;
	cxa_xmega_eventSystem_eventChannel_t triggerEventChan;
};


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes the specified capture channel of the given timer.
 *
 * @param[in] ccIn pointer to the pre-allocated capture channel object
 * @param[in] chanIn the desired capture channel (A || B || C || D for Timer 0s, A || B for Timer 1s)
 * @param[in] timerIn the desired timer
 * @param[in] capTriggerIn the pre-configured event channel which should be used to trigger the capture
 * @param[in] evDelayIn true if there should be a delay of one peripheral clock cycle between when an event
 *		occurs and when the actual capture occurs. This is useful when chaining 16-bit timers and one must
 *		account for the propagation delay of the event between the lower and upper timer.
 */
void cxa_xmega_timer16_captureChannel_init(cxa_xmega_timer16_captureChannel_t *const ccIn, const cxa_xmega_timer16_captureChannel_enum_t chanIn, const cxa_xmega_timer16_t timerIn, const cxa_xmega_eventSystem_eventChannel_t capTriggerIn, bool evDelayIn);


/**
 * @public
 * @brief Returns the event channel used to trigger this capture.
 * 
 * @param[in] ccIn pointer to the pre-initialized capture channel object
 *
 * @return the event channel which is currently being used to trigger this capture
 */
cxa_xmega_eventSystem_eventChannel_t cxa_xmega_timer16_captureChannel_getTriggerEventChannel(cxa_xmega_timer16_captureChannel_t *const ccIn);


/**
 * @public
 * @brief Returns the last timer value captured by this capture channel.
 *
 * @param[in] ccIn pointer to the pre-initialized capture channel object
 *
 * @return the timer value when this capture channel was last triggered
 */
uint16_t cxa_xmega_timer16_captureChannel_getLastCaptureVal(cxa_xmega_timer16_captureChannel_t *const ccIn);


#endif // CXA_XMEGA_TIMER16_CAPTURE_CHANNEL_H_
