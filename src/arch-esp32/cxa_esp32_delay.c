/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_delay.h"


// ******** includes ********
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


// ******** local macro definitions ********
#define NOP() 									asm volatile ("nop")


// ******** local type definitions ********


// ******** local function prototypes ********
static unsigned long IRAM_ATTR micros();
static void IRAM_ATTR delayMicroseconds(uint32_t us);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_delay_ms(uint16_t delay_msIn)
{
	vTaskDelay(delay_msIn / portTICK_PERIOD_MS);
}


void cxa_delay_us(uint32_t delay_usIn)
{
	delayMicroseconds(delay_usIn);
}


// ******** local function implementations ********
// taken from arduino-esp32 core
static unsigned long IRAM_ATTR micros()
{
    return (unsigned long) (esp_timer_get_time());
}

// taken from arduino-esp32 core
static void IRAM_ATTR delayMicroseconds(uint32_t us)
{
    uint32_t m = micros();
    if(us){
        uint32_t e = (m + us);
        if(m > e){ //overflow
            while(micros() > e){
                NOP();
            }
        }
        while(micros() < e){
            NOP();
        }
    }
}
