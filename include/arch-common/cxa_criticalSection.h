/**
 * @file
 * This file contains functions for creating critical sections. A critical section is
 * a section of code which, during execution, should be safe from context switches and interrupts.
 * A critical section is guaranteed to be executed pseudo-atomically, from start to finish,
 * without interruption. <b>Critical sections can be nested.</b>
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * cxa_criticalSection_enter();
 *
 * // do something critical
 * fputs("2", stdout);
 *
 * cxa_criticalSection_exit();
 * @endcode
 *
 *
 * #### Pre-enter / Post-exit Callbacks: ####
 * In certain cases, sections of code must be executed BEFORE entering a critical section
 * and AFTER exiting a critical section. (eg. asserting an RTS line to indicate that
 * no serial data reception will be possible during a critical section and de-asserting
 * that same line upon exit from the critical section)
 *
 * This functionality is provided through the ::cxa_criticalSection_addCallback function.
 * @code
 * // add callbacks to be notified when entering/exiting a critical section
 * cxa_criticalSection_addCallback(cb_enter, cb_exit, NULL);
 *
 * ...
 *
 * cxa_criticalSection_enter();
 *
 * // do something critical
 * fputs("2", stdout);
 *
 * cxa_criticalSection_exit();
 *
 * ...
 *
 * static void cb_enter(void *userVarIn){ fputs("1", stdout); }
 * static void cb_exit(void *userVarIn){ fputs("3", stdout); }
 * @endcode
 *
 * produces the output:
 *
 *     123 
 *
 *
 * @anchor externalEntry
 * #### External entry into a critical section (interrupts): ####
 * In some circumstances, what is essentially a critical section, can be entered without
 * first calling ::cxa_criticalSection_enter (eg. an interrupt routine). In this case,
 * we still want to call our critical section callbacks (eg. to assert an RTS line), even
 * if we already are in the critical section.
 * This can be achieved by wrapping your ISRs in ::cxa_criticalSection_notifyExternal_enter
 * and ::cxa_criticalSection_notifyExternal_exit.
 * @code
 * ISR(vectIn)
 * {
 *    cxa_criticalSection_notifyExternal_enter();
 *
 *    ...
 *
 *    cxa_criticalSection_notifyExternal_exit();
 * }
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
#ifndef CXA_CRITICAL_SECTION_H_
#define CXA_CRITICAL_SECTION_H_


// ******** includes ********


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief Callback for 'criticalSection_enter' and 'criticalSection_exit' events.
 * This is a callback prototype for a function that is called BEFORE entry into
 * a critical section and AFTER exit from a critical section.
 *
 * @param[in] userVarIn the user variable passed to ::cxa_criticalSection_addCallback
 */
typedef void (*cxa_criticalSection_cb_t)(void *userVarIn);


// ******** global function prototypes ********
/**
 * @public
 * @brief Called to enter a critical section
 * If not currently executing within a critical section,
 * will call all registered critical section callbacks, then take 
 * whatever steps are necessary to ensure atomic execution until
 * a corresponding call to ::cxa_criticalSection_exit (eg. disable
 * interrupts, stop a RTOS scheduler, etc). If currently in a critical
 * section this function essentially returns immediately.
 */
void cxa_criticalSection_enter(void);


/**
 * @public
 * @brief Called to exit a critical section
 * Should only be called from within a critical section (must
 * be called AFTER a call to ::cxa_criticalSection_enter).
 * If this is the outermost call to exit a critical section,
 * this function will take whatever steps are necessary to restore
 * the system to the execution state immediately preceding the first
 * call to ::cxa_criticalSection_enter (eg. re-enable interrupts, restart
 * RTOS scheduler, etc). If this is NOT the outermost call to exit a critical
 * section (eg. many nested called to enter and exit), this function
 * essentially returns immediately.
 */
void cxa_criticalSection_exit(void);


/**
 * @public
 * @brief Adds 'criticalSection_enter' and 'criticalSection_exit' callbacks.
 * Adds callbacks which will be called BEFORE entry into a critical section
 * and AFTER exit from a critical section.
 * @note some architectural implementations only support a fixed number of critical section callbacks
 *
 * @param[in] cb_preEnterIn callback that will be called BEFORE entry into a critical section
 * @param[in] cb_postExitIn callback that will be called AFTER exit from a critical section
 * @param[in] userVarIn the user variable which should be passed to the provided callbacks
 */
void cxa_criticalSection_addCallback(cxa_criticalSection_cb_t cb_preEnterIn, cxa_criticalSection_cb_t cb_postExitIn, void *userVarIn);


/**
 * @public
 * @brief Used to indicate to the critical section system that an external critical section has been entered.
 * See @ref externalEntry "External entry into a critical section (interrupts):"
 */
void cxa_criticalSection_notifyExternal_enter(void);


/**
 * @public
 * @brief Used to indicate to the critical section system that an external critical section has been exited.
 * See @ref externalEntry "External entry into a critical section (interrupts):"
 */
void cxa_criticalSection_notifyExternal_exit(void);


#endif // CXA_CRITICAL_SECTION_H_
