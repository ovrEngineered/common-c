/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
 
/**
 * @file
 * This file contains an implementation of a mPutF compliant assert framework.
 * Asserts are simple run-time checks of input arguments and conditions to
 * verify compliance with expected ranges and values. If an assert fails,
 * it will halt execution of the program and optionally, print information
 * about the location/cause.
 *
 * @note This object should work across all architecture-specific implementations
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * // nothing happens
 * cxa_assert(true);
 *
 * // halts execution and optionally prints location/cause
 * cxa_assert(false);
 * @endcode
 */
#ifndef CXA_ASSERT_H_
#define CXA_ASSERT_H_


// ******** includes ********
#include <stddef.h>
#include <stdio.h>
#include <cxa_config.h>
#ifdef CXA_ASSERT_GPIO_FLASH_ENABLE
	#include <cxa_gpio.h>
#endif
#include <cxa_ioStream.h>


#ifdef __cplusplus
extern "C" {
#endif


// ********* global macro definitions ********
#ifndef CXA_ASSERT_EXIT_FUNC
#define CXA_ASSERT_EXIT_FUNC(exitStatusIn)		exit(exitStatusIn)
#endif


#ifdef CXA_ASSERT_LINE_NUM_ENABLE
	/**
	 * @public
	 * @brief Asserts that the provided condition is true.
	 * If it isn't, prints
	 *
	 *     "\r\n**assert**\r\nloc: <file>:<lineNum>\r\n"
	 *
	 * where <file> and <lineNum> is the filename and line number from
	 * where this function was called.
	 *
	 * @note Enable this functionality by defining CXA_ASSERT_LINE_NUM_ENABLE
	 * in your cxa_config.h file
	 *
	 * @param[in] condIn the assertion condition (should be true)
	 */
	#define cxa_assert(condIn)					if( !(condIn) ) cxa_assert_impl(NULL, __FILE__, __LINE__);

	/**
	 * @public
	 * @brief Asserts that the provided condition is true.
	 * If it isn't, prints
	 *
	 *     "\r\n**assert**\r\nloc: <file>:<lineNum>\r\nmsg: <msg>\r\n"
	 *
	 * where <file> and <lineNum> is the filename and line number from
	 * where this function was called, and <msg> is the second
	 * parameter to this function.
	 *
	 * @param[in] condIn the assertion condition (should be true)
	 * @param[in] msgIn the message that should be displayed
	 */
	#ifdef CXA_ASSERT_MSG_ENABLE
		#define cxa_assert_msg(condIn, msgIn)		if( !(condIn) ) cxa_assert_impl((msgIn), __FILE__, __LINE__);
	#else
		#define cxa_assert_msg(condIn, msgIn)		if( !(condIn) ) cxa_assert_impl(NULL, __FILE__, __LINE__);
	#endif

    /**
     * @public
     * @brief Immediately asserts and prints a message with same format
     *      as @ref cxa_assert_msg
     *
     * @param[in] msgIn the message that should be displayed
     */
	#ifdef CXA_ASSERT_MSG_ENABLE
    	#define cxa_assert_failWithMsg(msgIn)       cxa_assert_impl((msgIn), __FILE__, __LINE__);
	#else
		#define cxa_assert_failWithMsg(msgIn)       cxa_assert_impl(NULL, __FILE__, __LINE__);
	#endif
#else
	/**
	 * @public
	 * @brief Asserts that the provided condition is true.
	 * If it isn't, simply prints
	 *
	 *     "\r\n**assert**\r\n"
	 *
	 * @param[in] condIn the assertion condition (should be true)
	 */
	#define cxa_assert(condIn)					if( !(condIn) ) cxa_assert_impl(NULL, NULL, 0);

	/**
	 * @public
	 * @brief Asserts that the provided condition is true.
	 * If it isn't, prints
	 *
	 *     "\r\n**assert**\r\nmsg: <msg>\r\n"
	 *
	 * where <msg> is the parameter to this function.
	 *
	 * @param[in] condIn the assertion condition (should be true)
	 * @param[in] msgIn the message that should be displayed
	 */
	#ifdef CXA_ASSERT_MSG_ENABLE
		#define cxa_assert_msg(condIn, msgIn)		if( !(condIn) ) cxa_assert_impl((msgIn), NULL, 0);
	#else
		#define cxa_assert_msg(condIn, msgIn)		if( !(condIn) ) cxa_assert_impl(NULL, NULL, 0);
	#endif

    /**
     * @public
     * @brief Immediately asserts and prints a message with same format
     *      as @ref cxa_assert_msg
     *
     * @param[in] msgIn the message that should be displayed
     */
	#ifdef CXA_ASSERT_MSG_ENABLE
    	#define cxa_assert_failWithMsg(msgIn)       cxa_assert_impl((msgIn), NULL, 0);
	#else
		#define cxa_assert_failWithMsg(msgIn)       cxa_assert_impl(NULL, NULL, 0);
	#endif
#endif


// ********* global type definitions *********
/**
 * @public
 * @brief A prototype for a callback that can be called when an assert occurs.
 */
typedef void (*cxa_assert_cb_t)(void);


// ********* global function prototypes ********
/**
 * @public
 * @brief Sets the ioStream that should be used to output assert messages.
 * Should be called prior to any cxa_assert* macro calls.
 * If not set, no assert message will be printed.
 *
 * @param[in] ioStreamIn the pre-configured ioStream which should
 * 		be used to print the assert message
 */
void cxa_assert_setIoStream(cxa_ioStream_t *const ioStreamIn);


/**
 * @public
 * @brief Sets a single callback that will be called once an assert
 * occurs _before_ entering the terminal while loop.
 *
 * This callback should generally be used to nicely shutdown any
 * processes.
 *
 * Note: this function _may_ be called in the main loop context
 *		or an an interrupt context. As such, it should be assumed
 *		that interrupts will forever more be disabled.
 *
 * @param[in] cbIn the callback
 */
void cxa_assert_setAssertCb(cxa_assert_cb_t cbIn);


#ifdef CXA_ASSERT_GPIO_FLASH_ENABLE
	/**
	 * @public
	 * @brief When CXA_ASSERT_GPIO_FLASH_ENABLE is enabled, allows a single
	 * GPIO to be flashed continuously to indicate to the outside
	 * world that something is wrong.
	 *
	 * If enabled and supplied, the GPIO flashing will replace a terminal call to exit()
	 *
	 * @param[in] gpioIn the pre-configured GPIO that will be forcefully
	 *		configured as an output and toggled continuously
	 *		(with a small delay between toggles)
	 */
	void cxa_assert_setAssertGpio(cxa_gpio_t *const gpioIn);
#endif


/**
 * @private
 * Used by cxa_assert* macros
 */
void cxa_assert_impl(const char *msgIn, const char *fileIn, const long int lineIn);


#ifdef __cplusplus
}
#endif
#endif // CXA_ASSERT_H_
