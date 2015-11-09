/**
 * Original Source:
 * https://github.com/esp8266/Arduino.git
 *
 * Copyright 2015 opencxa.org
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
 *
 * @author Christopher Armenio
 */


// ******** includes ********
#include <cxa_assert.h>
#include <cont.h>
#include <ets_sys.h>
#include <user_interface.h>


// ******** local macro definitions ********
#define LOOP_TASK_PRIORITY					0


// ******** local type definitions ********


// ******** local function prototypes ********
extern void setup(void);
extern void loop(void);

static void esp_schedule(void);
static void init_done(void);
static void loop_wrapper(void);
static void loop_task(os_event_t *events);


// ********  local variable declarations *********
cont_t g_cont __attribute__ ((aligned (16)));
static os_event_t g_loop_queue[1];


// ******** global function implementations ********
void user_init(void)
{
	cont_init(&g_cont);

	// setup our run loop
	system_os_task(loop_task, LOOP_TASK_PRIORITY, g_loop_queue, (sizeof(g_loop_queue)/(sizeof(*g_loop_queue))));

	system_init_done_cb(&init_done);
}


void preloop_update_frequency(void) __attribute__((weak));
void preloop_update_frequency(void)
{
#if defined(F_CPU) && (F_CPU == 160000000L)
    REG_SET_BIT(0x3ff00014, BIT(0));
    ets_update_cpu_frequency(160);
#endif
}


// ******** local function implementations ********
static void esp_schedule(void)
{
    system_os_post(LOOP_TASK_PRIORITY, 0, 0);
}


static void init_done(void)
{
    system_set_os_print(1);
    esp_schedule();
}


static void loop_wrapper(void)
{
    static bool setup_done = false;
    if(!setup_done)
    {
        setup();
        setup_done = true;
    }
    preloop_update_frequency();
    loop();
    esp_schedule();
}


static void loop_task(os_event_t *events)
{
    cont_run(&g_cont, &loop_wrapper);
    if( cont_check(&g_cont) != 0 )
    {
        ets_printf("\r\nsketch stack overflow detected\r\n");
        while(1) { }
    }
}
