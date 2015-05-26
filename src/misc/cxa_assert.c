/**
 * Copyright 2013 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "cxa_assert.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdlib.h>
#include <cxa_delay.h>


// ******** local macro definitions ********
#define EXIT_STATUS					84
#define GPIO_FLASH_DELAY_MS			100
#define ASSERT_TEXT					"**assert**"
#define PREAMBLE_LOCATION			"loc: "
#define PREAMBLE_MESSAGE			"msg: "


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********
static FILE *fd_msg = NULL;
static cxa_assert_cb_t cb = NULL;
#ifdef CXA_ASSERT_GPIO_FLASH_ENABLE
	static cxa_gpio_t *gpio;
#endif


// ******** global function implementations ********
void cxa_assert_setFileDescriptor(FILE *fileIn)
{
	fd_msg = fileIn;
}


void cxa_assert_setAssertCb(cxa_assert_cb_t cbIn)
{
	cb = cbIn;
}


#ifdef CXA_ASSERT_GPIO_FLASH_ENABLE
	void cxa_assert_setAssertGpio(cxa_gpio_t *const gpioIn)
	{
		gpio = gpioIn;
	}
#endif


#ifdef CXA_ASSERT_LINE_NUM_ENABLE
	void cxa_assert_impl(const char *fileIn, const long int lineIn)
	{
		if( fd_msg != NULL )
		{
			fprintf(fd_msg, "\r\n%s\r\n%s%s:%ld\r\n",
					ASSERT_TEXT,
					PREAMBLE_LOCATION,
					fileIn,
					lineIn);
			fflush(fd_msg);
		}
		if( cb != NULL ) cb();
		
		#ifdef CXA_ASSERT_GPIO_FLASH_ENABLE
			if( gpio != NULL )
			{
				cxa_gpio_setDirection(gpio, CXA_GPIO_DIR_OUTPUT);
				while(1)
				{
					cxa_gpio_toggle(gpio);
					cxa_delay_ms(GPIO_FLASH_DELAY_MS);
				}
			} else exit(EXIT_STATUS);
		#else
			exit(EXIT_STATUS);
		#endif
	}
#else
	void cxa_assert_impl(void)
	{
		if( fd_msg != NULL )
		{
			fputs("\r\n", fd_msg);
			fputs(ASSERT_TEXT, fd_msg);
			fputs("\r\n", fd_msg);
			fflush(fd_msg);
		}
		if( cb != NULL ) cb();
		
		#ifdef CXA_ASSERT_GPIO_FLASH_ENABLE
			if( gpio != NULL )
			{
				cxa_gpio_setDirection(gpio, CXA_GPIO_DIR_OUTPUT);
				while(1)
				{
					cxa_gpio_toggle(gpio);
					cxa_delay_ms(GPIO_FLASH_DELAY_MS);
				}
			} else exit(EXIT_STATUS);
		#else
			exit(EXIT_STATUS);
		#endif
	}
#endif


#if defined (CXA_ASSERT_MSG_ENABLE) && defined (CXA_ASSERT_LINE_NUM_ENABLE)
	void cxa_assert_impl_msg(const char *msgIn, const char *fileIn, const long int lineIn)
	{
		if( fd_msg != NULL )
		{
			fprintf(fd_msg, "\r\n%s\r\n%s%s:%ld\r\n%s%s",
					ASSERT_TEXT,
					PREAMBLE_LOCATION,
					fileIn,
					lineIn,
					PREAMBLE_MESSAGE,
					msgIn);
			fflush(fd_msg);
		}
		if( cb != NULL ) cb();
		
		#ifdef CXA_ASSERT_GPIO_FLASH_ENABLE
			if( gpio != NULL )
			{
				cxa_gpio_setDirection(gpio, CXA_GPIO_DIR_OUTPUT);
				while(1)
				{
					cxa_gpio_toggle(gpio);
					cxa_delay_ms(GPIO_FLASH_DELAY_MS);
				}
			} else exit(EXIT_STATUS);
		#else
			exit(EXIT_STATUS);
		#endif
	}
#elif defined (CXA_ASSERT_MSG_ENABLE) && !(defined (CXA_ASSERT_LINE_NUM_ENABLE))
	void cxa_assert_impl_msg(const char *msgIn)
	{
		if( fd_msg != NULL )
		{
			fputs("\r\n ", fd_msg);
			fputs(ASSERT_TEXT, fd_msg);
			fputs("\r\n", fd_msg);
			fputs(PREAMBLE_MESSAGE, fd_msg);
			fputs(msgIn, fd_msg);
			fputs("\r\n", fd_msg);
			fflush(fd_msg);
		}
		if( cb != NULL ) cb();
		
		#ifdef CXA_ASSERT_GPIO_FLASH_ENABLE
			if( gpio != NULL )
			{
				cxa_gpio_setDirection(gpio, CXA_GPIO_DIR_OUTPUT);
				while(1)
				{
					cxa_gpio_toggle(gpio);
					cxa_delay_ms(GPIO_FLASH_DELAY_MS);
				}
			} else exit(EXIT_STATUS);
		#else
			exit(EXIT_STATUS);
		#endif
	}
#endif




// ******** local function implementations ********

