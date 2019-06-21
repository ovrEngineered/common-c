/*
 * Copyright 2019 ovrEngineered, LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining 
 * a copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
 * copies of the Software, and to permit persons to whom the Software is furnished to 
 * do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all 
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_SILABSBGAPI_BTLE_PERIPHERAL_H_
#define CXA_SILABSBGAPI_BTLE_PERIPHERAL_H_


// ******** includes ********
#include <gecko_bglib.h>

#include <stdbool.h>
#include <cxa_btle_peripheral.h>


// ******** global macro definitions ********
#ifndef CXA_SILABSBGAPI_BTLE_MAXNUM_CONNS
#define CXA_SILABSBGAPI_BTLE_MAXNUM_CONNS			3
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_siLabsBgApi_btle_peripheral cxa_siLabsBgApi_btle_peripheral_t;


/**
 * @private
 */
typedef struct
{
	cxa_btle_uuid_t serviceUuid;
	cxa_btle_uuid_t charUuid;

	uint16_t handle;
}cxa_siLabsBgApi_btle_handleCharMapEntry_t;


/**
 * @private
 */
typedef struct
{
	uint8_t handle;
	cxa_eui48_t macAddress;
}cxa_siLabsBgApi_btle_handleMacMapEntry_t;


/**
 * @private
 */
struct cxa_siLabsBgApi_btle_peripheral
{
	cxa_btle_peripheral_t super;

	cxa_array_t handleCharMap;
	cxa_siLabsBgApi_btle_handleCharMapEntry_t handleCharMap_raw[CXA_BTLE_PERIPHERAL_MAXNUM_CHAR_ENTRIES];

	cxa_array_t handleMacMap;
	cxa_siLabsBgApi_btle_handleMacMapEntry_t handleMacMap_raw[CXA_SILABSBGAPI_BTLE_MAXNUM_CONNS];
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_siLabsBgApi_btle_peripheral_init(cxa_siLabsBgApi_btle_peripheral_t *const btlepIn, int threadIdIn);


/**
 * @public
 */
void cxa_siLabsBgApi_btle_peripheral_registerHandle(cxa_siLabsBgApi_btle_peripheral_t *const btlepIn,
													const char *const serviceUuidStrIn,
													const char *const charUuidStrIn,
													uint16_t handleIn);


/**
 * @protected
 *
 * @return true if this event was handled
 */
bool cxa_siLabsBgApi_btle_peripheral_handleBgEvent(cxa_siLabsBgApi_btle_peripheral_t *const btlepIn, struct gecko_cmd_packet *evt);


#endif
