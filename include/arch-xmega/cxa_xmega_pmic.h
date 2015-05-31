/**
 * @file
 * This file contains functions for configuring the Programmable Multi-level Interrupt Controller (PMIC)
 * for the XMega processor. The PMIC is responsible for configuring and handling all interrupts on the
 * XMega device.
 *
 * @note This file contains functionality restricted to the CXA Atmel XMega implementation.
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * // should be one of the first calls in your program
 * cxa_xmega_pmic_init();
 *
 * // enable only interrupts that have been configured to have a HIGH priority
 * cxa_xmega_pmic_enableInterruptLevel(CXA_XMEGA_PMIC_INTLEVEL_HIGH);
 *
 * // setup our USART (for example)
 * 
 * // enable global interrupts
 * cxa_xmega_pmic_enableGlobalInterrupts();
 *
 * ...
 *
 * // interrupt handler for USART
 * CXA_XMEGA_ISR_START(USARTD0_RXC_vect)
 * {
 *    // receive a byte
 * }CXA_XMEGA_ISR_END
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
#ifndef CXA_XMEGA_PMIC_H_
#define CXA_XMEGA_PMIC_H_


// ******** includes ********
#include <stdbool.h>
#include <avr/interrupt.h>
#include <cxa_criticalSection.h>


// ******** global macro definitions ********
/**
 * @public
 * @brief A macro which should be present at the beginning of every ISR.
 * This macro immediately notifies all critical section callbacks that
 * we are in an interrupt (and essentially a critical section).
 *
 * Yes, there might be a lot of code to execute here, and yes we should
 * strive to keep ISRs as short as possible. However, for critical functions
 * like flow control for USARTs, we need to assert flow control lines while
 * in an ISR.
 *
 * Every instance of this macro must have a matching ::CXA_XMEGA_ISR_END
 *
 * @param[in] vectIn the interrupt vector for this ISR (eg. USARTC0_RXC_vect)
 */
#define CXA_XMEGA_ISR_START(vectIn)							\
ISR(vectIn)													\
{															\
	cxa_criticalSection_notifyExternal_enter();


/**
 * @public
 * @brief A macro which should be present at the end of every ISR.
 * ISR should be started with ::CXA_XMEGA_ISR_START
 */
#define CXA_XMEGA_ISR_END									\
	cxa_criticalSection_notifyExternal_exit();				\
}


// ******** global type definitions *********
/**
 * @public
 * @brief Enumeration containing the 3 possible interrupt levels for any given
 *		ISR on the XMega.
 */
typedef enum
{
	CXA_XMEGA_PMIC_INTLEVEL_LOW,								///< Low interrupt level
	CXA_XMEGA_PMIC_INTLEVEL_MED,								///< Medium interrupt level
	CXA_XMEGA_PMIC_INTLEVEL_HIGH								///< High interrupt level
}cxa_xmega_pmic_interruptLevel_t;


// ******** global function prototypes ********
/**
 * @public
 * @brief Initializes the interrupt control system.
 * This should be one of the first calls of your program.
 */
void cxa_xmega_pmic_init(void);

/**
 * @public
 * @brief Determines if the PMIC system has been initialized.
 *
 * @return true if the PMIC system has been initialized, false if not
 */
bool cxa_xmega_pmic_hasBeenInitialized(void);

/**
 * @public
 * @brief Enables the given interrupt level.
 * One or multiple calls (different levels) to this function should immediately follow
 * your call to ::cxa_xmega_pmic_init at the beginning of your program.
 *
 * @note Although the naming of this function connotes that the given
 *		interrupt level will be enabled, global interrupts must also be
 *		enabled before ANY interrupts occur. Enable global interrupts using
 *		::cxa_xmega_pmic_enableGlobalInterrupts
 *
 * @param[in] levelIn the desired interrupt level to enable
 */
void cxa_xmega_pmic_enableInterruptLevel(const cxa_xmega_pmic_interruptLevel_t levelIn);


/**
 * @public
 * @brief Enables global interrupts.
 * If any interrupt levels were enabled using ::cxa_xmega_pmic_enableInterruptLevel
 * those interrupts may now occur (depending upon the configuration of your peripherals).
 */
void cxa_xmega_pmic_enableGlobalInterrupts(void);


/**
 * @public
 * @brief Disables global interrupts.
 * If any interrupt levels were enabled using ::cxa_xmega_pmic_enableInterruptLevel
 * those interrupts may will not occur until a call to ::cxa_xmega_pmic_enableGlobalInterrupts
 */
void cxa_xmega_pmic_disableGlobalInterrupts(void);


#endif // CXA_XMEGA_PMIC_H_