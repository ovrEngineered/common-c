/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_SILABSBGAPI_BTLE_CENTRAL_H_
#define CXA_SILABSBGAPI_BTLE_CENTRAL_H_


// ******** includes ********
#include <cxa_config.h>
#ifndef CXA_SILABSBGAPI_MODE_SOC
#include <gecko_bglib.h>
#else
#include "bg_types.h"
#include "native_gecko.h"
#include "infrastructure.h"
#endif

#include <cxa_btle_central.h>
#include <cxa_btle_uuid.h>
#include <cxa_eui48.h>
#include <cxa_ioStream_peekable.h>
#include <cxa_siLabsBgApi_btle_connection.h>


// ******** global macro definitions ********
#ifndef CXA_SILABSBGAPI_BTLE_CENTRAL_MAXNUM_CONNS
	#define CXA_SILABSBGAPI_BTLE_CENTRAL_MAXNUM_CONNS						2
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_siLabsBgApi_btle_central cxa_siLabsBgApi_btle_central_t;


/**
 * @private
 */
struct cxa_siLabsBgApi_btle_central
{
	cxa_btle_central_t super;

	int threadId;

	bool isConnectionInProgress;

	cxa_siLabsBgApi_btle_connection_t conns[CXA_SILABSBGAPI_BTLE_CENTRAL_MAXNUM_CONNS];
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_siLabsBgApi_btle_central_init(cxa_siLabsBgApi_btle_central_t *const btlecIn, int threadIdIn);


/**
 * @public
 */
bool cxa_siLabsBgApi_btle_central_setConnectionInterval(cxa_siLabsBgApi_btle_central_t *const btlecIn, cxa_eui48_t *const targetConnectionAddressIn, uint16_t connectionInterval_msIn);


/**
 * @protected
 *
 * @return true if this event was handled
 */
bool cxa_siLabsBgApi_btle_central_handleBgEvent(cxa_siLabsBgApi_btle_central_t *const btlecIn, struct gecko_cmd_packet *evt);

#endif
