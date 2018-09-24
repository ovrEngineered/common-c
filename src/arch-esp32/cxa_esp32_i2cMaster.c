/**
 * Copyright 2017 opencxa.org
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
#include "cxa_esp32_i2cMaster.h"


#include <stdio.h>
#include "driver/periph_ctrl.h"
#include "freertos/FreeRTOS.h"

#include <cxa_assert.h>
#include <cxa_delay.h>


#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define BUS_RESET_TOGGLE_PERIOD_MS			1
#define ACK_VAL    							0x0         /*!< I2C ack value */
#define NACK_VAL   							0x1         /*!< I2C nack value */
#define INTERRUPT_LEVEL						ESP_INTR_FLAG_LOWMED


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_readBytes(cxa_i2cMaster_t *const superIn, uint8_t addressIn, uint8_t sendStopIn, size_t numBytesToReadIn);
static void scm_readBytesWithControlBytes(cxa_i2cMaster_t *const i2cIn,
										 uint8_t addressIn, uint8_t sendStopIn,
										 cxa_fixedByteBuffer_t *const controlBytesIn,
										 size_t numBytesToReadIn);
static void scm_writeBytes(cxa_i2cMaster_t *const superIn, uint8_t addressIn, uint8_t sendStopIn, cxa_fixedByteBuffer_t *const writeBytesIn);
static void scm_resetBus(cxa_i2cMaster_t *const superIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_esp32_i2cMaster_init(cxa_esp32_i2cMaster_t *const i2cIn, const i2c_port_t i2cPortIn,
							 const gpio_num_t pinNum_sdaIn, const gpio_num_t pinNum_sclIn,
							 const bool enablePullUpsIn, uint32_t busFreq_hzIn)
{
	cxa_assert(i2cIn);
	
	// save our references
	i2cIn->i2cPort = i2cPortIn;

	// initialize our hardware
	i2cIn->conf.mode = I2C_MODE_MASTER;
	i2cIn->conf.sda_io_num = pinNum_sdaIn;
	i2cIn->conf.sda_pullup_en = enablePullUpsIn ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
	i2cIn->conf.scl_io_num = pinNum_sclIn;
	i2cIn->conf.scl_pullup_en = enablePullUpsIn ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
	i2cIn->conf.master.clk_speed = busFreq_hzIn;
	i2c_param_config(i2cIn->i2cPort, &i2cIn->conf);

	// no buffer needed in master mode
	i2c_driver_install(i2cIn->i2cPort, i2cIn->conf.mode, 0, 0, INTERRUPT_LEVEL);

	// initialize our super class
	cxa_i2cMaster_init(&i2cIn->super, scm_readBytes, scm_readBytesWithControlBytes, scm_writeBytes, scm_resetBus);
}


// ******** local function implementations ********
static void scm_readBytes(cxa_i2cMaster_t *const superIn, uint8_t addressIn, uint8_t sendStopIn, size_t numBytesToReadIn)
{
	cxa_esp32_i2cMaster_t* i2cIn = (cxa_esp32_i2cMaster_t*)superIn;
	cxa_assert(i2cIn);

	i2c_cmd_handle_t handle = i2c_cmd_link_create();

	// send our address
	i2c_master_start(handle);
	i2c_master_write_byte(handle, (addressIn << 1) | I2C_MASTER_READ, true);

	// setup our read
	uint8_t fbb_readBytes_raw[numBytesToReadIn];
	i2c_master_read(handle, fbb_readBytes_raw, numBytesToReadIn, true);
	if( sendStopIn ) i2c_master_stop(handle);

	// commit the read to the bus
	esp_err_t ret = i2c_master_cmd_begin(i2cIn->i2cPort, handle, 500 * portTICK_RATE_MS);

	// create our return buffer
	cxa_fixedByteBuffer_t fbb_readBytes;
	cxa_fixedByteBuffer_init_inPlace(&fbb_readBytes, numBytesToReadIn, fbb_readBytes_raw, sizeof(fbb_readBytes_raw));

	// return the read values
	cxa_i2cMaster_notify_readComplete(&i2cIn->super, (ret == ESP_OK), (ret == ESP_OK) ? &fbb_readBytes : NULL);

	i2c_cmd_link_delete(handle);
}


static void scm_readBytesWithControlBytes(cxa_i2cMaster_t *const superIn,
										 uint8_t addressIn, uint8_t sendStopIn,
										 cxa_fixedByteBuffer_t *const controlBytesIn,
										 size_t numBytesToReadIn)
{
	cxa_esp32_i2cMaster_t* i2cIn = (cxa_esp32_i2cMaster_t*)superIn;
	cxa_assert(i2cIn);

	i2c_cmd_handle_t handle = i2c_cmd_link_create();

	// send our address
	i2c_master_start(handle);
	i2c_master_write_byte(handle, (addressIn << 1) | I2C_MASTER_WRITE, true);

	// write our control bytes
	i2c_master_write(handle, cxa_fixedByteBuffer_get_pointerToIndex(controlBytesIn, 0), cxa_fixedByteBuffer_getSize_bytes(controlBytesIn), true);

	// now restart and send our address
	i2c_master_start(handle);
	i2c_master_write_byte(handle, (addressIn << 1) | I2C_MASTER_READ, true);

	// setup our read
	uint8_t fbb_readBytes_raw[numBytesToReadIn];
	if( numBytesToReadIn > 1 ) i2c_master_read(handle, fbb_readBytes_raw, numBytesToReadIn-1, ACK_VAL);
	i2c_master_read_byte(handle, &fbb_readBytes_raw[numBytesToReadIn-1], NACK_VAL);
	if( sendStopIn ) i2c_master_stop(handle);

	// commit the read to the bus
	esp_err_t ret = i2c_master_cmd_begin(i2cIn->i2cPort, handle, 500 * portTICK_RATE_MS);

	// create our return buffer
	cxa_fixedByteBuffer_t fbb_readBytes;
	cxa_fixedByteBuffer_init_inPlace(&fbb_readBytes, numBytesToReadIn, fbb_readBytes_raw, sizeof(fbb_readBytes_raw));

	// return the read values
	cxa_i2cMaster_notify_readComplete(&i2cIn->super, (ret == ESP_OK), (ret == ESP_OK) ? &fbb_readBytes : NULL);

	i2c_cmd_link_delete(handle);
}


static void scm_writeBytes(cxa_i2cMaster_t *const superIn, uint8_t addressIn, uint8_t sendStopIn, cxa_fixedByteBuffer_t *const writeBytesIn)
{
	cxa_esp32_i2cMaster_t* i2cIn = (cxa_esp32_i2cMaster_t*)superIn;
	cxa_assert(i2cIn);

	i2c_cmd_handle_t handle = i2c_cmd_link_create();

	// send our address
	i2c_master_start(handle);
	i2c_master_write_byte(handle, (addressIn << 1) | I2C_MASTER_WRITE, true);

	// setup our write
	i2c_master_write(handle, cxa_fixedByteBuffer_get_pointerToIndex(writeBytesIn, 0), cxa_fixedByteBuffer_getSize_bytes(writeBytesIn), true);
	if( sendStopIn ) i2c_master_stop(handle);

	// commit the write to the bus
	esp_err_t ret = i2c_master_cmd_begin(i2cIn->i2cPort, handle, 500 * portTICK_RATE_MS);

	// return our success or failure
	cxa_i2cMaster_notify_writeComplete(&i2cIn->super, (ret == ESP_OK));

	i2c_cmd_link_delete(handle);
}


static void scm_resetBus(cxa_i2cMaster_t *const superIn)
{
	cxa_esp32_i2cMaster_t* i2cIn = (cxa_esp32_i2cMaster_t*)superIn;
	cxa_assert(i2cIn);

	// remove our i2c driver
	i2c_driver_delete(i2cIn->i2cPort);

	// reset our pin matrix
	gpio_matrix_out(i2cIn->conf.scl_io_num, 0x100, false, false);
	gpio_matrix_out(i2cIn->conf.sda_io_num, 0x100, false, false);

	// configure for output mode so we can manually clock
	gpio_set_level(i2cIn->conf.scl_io_num, 1);
	gpio_pad_select_gpio(i2cIn->conf.scl_io_num);
	gpio_pad_select_gpio(i2cIn->conf.sda_io_num);
	gpio_set_direction(i2cIn->conf.scl_io_num, GPIO_MODE_OUTPUT);
	gpio_set_direction(i2cIn->conf.sda_io_num, GPIO_MODE_INPUT);

	// cycle up to 9 times...per I2C specifications
	for( int i = 0; i < 9; i++ )
	{
		cxa_delay_ms(BUS_RESET_TOGGLE_PERIOD_MS);
		gpio_set_level(i2cIn->conf.scl_io_num, 0);
		cxa_delay_ms(BUS_RESET_TOGGLE_PERIOD_MS);
		gpio_set_level(i2cIn->conf.scl_io_num, 1);
		cxa_delay_ms(BUS_RESET_TOGGLE_PERIOD_MS);

		// check to see if it's been released
		if( gpio_get_level(i2cIn->conf.sda_io_num) ) break;
	}

	// reconfigure for i2c
	gpio_matrix_out(i2cIn->conf.scl_io_num, 0x100, false, false);
	gpio_matrix_out(i2cIn->conf.sda_io_num, 0x100, false, false);
	i2c_param_config(i2cIn->i2cPort, &i2cIn->conf);

	// reinstall our driver
	i2c_driver_install(i2cIn->i2cPort, i2cIn->conf.mode, 0, 0, INTERRUPT_LEVEL);

	cxa_delay_ms(500);
}
