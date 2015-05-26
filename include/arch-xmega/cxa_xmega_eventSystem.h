/**
 * @file
 * This file contains functions for configuring the event system of the XMega processor.
 * The event system is a powerful mechanism for creating complex actions that occur without
 * invervention from the CPU (eg. linking two 16-bit timers to create a 32-bit timer).
 *
 * In general, an event channel has a single source and can be "subscribed to" by multiple
 * peripherals to trigger some sort of hardware-defined action.
 *
 * @note This file contains functionality restricted to the CXA Atmel XMega implementation.
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * // sets up event channel 0 to respond to manually triggered events only
 * cxa_xmega_eventSystem_initChannel_manualOnly(CXA_XMEGA_EVENT_CHAN_0);
 *
 * ...
 *
 * // now take some steps to configure what should happen when event channel 0
 * // is triggered (eg. timer capture, increment  a counter, etc). See documentation
 * // for the desired peripheral
 *
 * ..
 *
 * // manually trigger the event
 * cxa_xmega_eventSystem_triggerEvents(CXA_XMEGA_EVENT_CHAN_0);
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
#ifndef CXA_XMEGA_EVENT_SYSTEM_H_
#define CXA_XMEGA_EVENT_SYSTEM_H_


// ******** includes ********
#include <stdint.h>
#include <cxa_xmega_timer16.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief Enumeration containing an element for each event channel (8 total)
 */
typedef enum
{
	CXA_XMEGA_EVENT_CHAN_0=0x01,										///< Event channel 0
	CXA_XMEGA_EVENT_CHAN_1=0x02,										///< Event channel 1
	CXA_XMEGA_EVENT_CHAN_2=0x04,										///< Event channel 2
	CXA_XMEGA_EVENT_CHAN_3=0x08,										///< Event channel 3
	CXA_XMEGA_EVENT_CHAN_4=0x10,										///< Event channel 4
	CXA_XMEGA_EVENT_CHAN_5=0x20,										///< Event channel 5
	CXA_XMEGA_EVENT_CHAN_6=0x40,										///< Event channel 6
	CXA_XMEGA_EVENT_CHAN_7=0x80											///< Event channel 7
}cxa_xmega_eventSystem_eventChannel_t;


/**
 * @public
 * @brief Enumeration consisting of possible events triggered by a timer
 */
typedef enum
{
	CXA_XMEGA_EVENT_SOURCE_TC_OVERFLOW=0x00,							///< Event triggered by an overflow of a 16-bit timer (TC0 or TC1)
}cxa_xmega_eventSystem_timerEvent_t;


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes the given event channel to be triggered by a provided timer event.
 * 
 * @param[in] chanIn the desired event channel to configure
 * @param[in] timerIn the specific timer that will be generating/trigger this event
 * @param[in] timerEvIn the event within the specified timer that will generate/trigger this event
 */
void cxa_xmega_eventSystem_initChannel_timerEvent(const cxa_xmega_eventSystem_eventChannel_t chanIn, const cxa_xmega_timer16_t timerIn, const cxa_xmega_eventSystem_timerEvent_t timerEvIn);


/**
 * @public
 * @brief Initializes the given event channel to be triggered manually.
 * Manual trigger of an event channel is accommplished through ::cxa_xmega_eventSystem_triggerEvents.
 *
 * @param[in] chanIn the desired event channel to configure
 */
void cxa_xmega_eventSystem_initChannel_manualOnly(const cxa_xmega_eventSystem_eventChannel_t chanIn);


/**
 * @public
 * @brief Returns an event channel that has not yet been initialized using one of the
 *		functions in this file. If no event channels are available (all are used), 
 *		this function will assert (halt execution).
 *
 * @return an unused event channel or function will assert if none are available
 */
cxa_xmega_eventSystem_eventChannel_t cxa_xmega_eventSystem_getUnusedEventChannel(void);


/**
 * @public
 * @brief Manually triggers an event on the specified channel.
 * Although channels can be configured to respond to manual events only
 * (through ::cxa_xmega_eventSystem_initChannel_manualOnly) <b>all</b>
 * event channels can be triggered manually. To enable triggering of
 * multiple events at the same time, this function accepts OR'd
 * event channels.
 *
 * @code
 * // trigger channel 0 and 1
 * cxa_xmega_eventSystem_triggerEvents(CXA_XMEGA_EVENT_CHAN_0 | CXA_XMEGA_EVENT_CHAN_1);
 * @endcode
 *
 * @param[in] eventstoTrigger OR'd event channels (::cxa_xmega_eventSystem_eventChannel_t)
 *		which should be manually triggered at the same time
 */
void cxa_xmega_eventSystem_triggerEvents(uint8_t eventsToTrigger);


#endif // CXA_XMEGA_EVENT_SYSTEM_H_
