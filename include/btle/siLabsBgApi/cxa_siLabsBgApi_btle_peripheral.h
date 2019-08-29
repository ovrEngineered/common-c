/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_SILABSBGAPI_BTLE_PERIPHERAL_H_
#define CXA_SILABSBGAPI_BTLE_PERIPHERAL_H_


// ******** includes ********
#include <cxa_config.h>
#ifndef CXA_SILABSBGAPI_MODE_SOC
#include <gecko_bglib.h>
#else
#include "bg_types.h"
#include "native_gecko.h"
#include "infrastructure.h"
#endif

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
	cxa_btle_peripheral_charEntry_t *charEntry;
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

	bool hasUserAdvertDataSet;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_siLabsBgApi_btle_peripheral_init(cxa_siLabsBgApi_btle_peripheral_t *const btlepIn, int threadIdIn);


/**
 * @public
 * This needs to be done _after_ all characteristic handlers have been registered
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
