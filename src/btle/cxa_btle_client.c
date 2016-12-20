/**
 * @copyright 2016 opencxa.org
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
#include "cxa_btle_client.h"


// ******** includes ********
#include <string.h>

#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_btle_client_init(cxa_btle_client_t *const btlecIn,
						  cxa_btle_client_scm_startScan_t scm_startScanIn,
						  cxa_btle_client_scm_stopScan_t scm_stopScanIn)
{
	cxa_assert(btlecIn);
	cxa_assert(scm_startScanIn);

	// save our references
	btlecIn->scms.startScan = scm_startScanIn;
	btlecIn->scms.stopScan = scm_stopScanIn;

	// clear our callbacks
	memset((void*)&btlecIn->cbs, 0, sizeof(btlecIn->cbs));
}


void cxa_btle_client_startScan_passive(cxa_btle_client_t *const btlecIn,
									   cxa_btle_client_cb_onAdvertRx_t cb_advIn,
									   cxa_btle_client_cb_onScanStartFail_t cb_scanStartFailIn,
									   void* userVarIn)
{
	cxa_assert(btlecIn);

	// save our callbacks
	btlecIn->cbs.onAdvert = cb_advIn;
	btlecIn->cbs.onScanStartFail = cb_scanStartFailIn;
	btlecIn->cbs.userVar = userVarIn;

	// start our scan
	cxa_assert(btlecIn->scms.startScan);
	btlecIn->scms.startScan(btlecIn, false);
}


void cxa_btle_client_startScan_active(cxa_btle_client_t *const btlecIn,
									  cxa_btle_client_cb_onAdvertRx_t cb_advIn,
									  cxa_btle_client_cb_onScanResponseRx_t cb_scanRespIn,
									  cxa_btle_client_cb_onScanStartFail_t cb_scanStartFailIn,
									  void* userVarIn)
{
	cxa_assert(btlecIn);

	// save our callbacks
	btlecIn->cbs.onAdvert = cb_advIn;
	btlecIn->cbs.onScanResp = cb_scanRespIn;
	btlecIn->cbs.onScanStartFail = cb_scanStartFailIn;
	btlecIn->cbs.userVar = userVarIn;

	// start our scan
	cxa_assert(btlecIn->scms.startScan);
	btlecIn->scms.startScan(btlecIn, false);
}


void cxa_btle_client_stopScan(cxa_btle_client_t *const btlecIn,
							  cxa_btle_client_cb_onScanStop_t cb_scanStopIn,
							  void* userVarIn)
{
	cxa_assert(btlecIn);

	// save our callbacks
	btlecIn->cbs.onScanStop = cb_scanStopIn;
	btlecIn->cbs.userVar = userVarIn;

	// stop our scan
	cxa_assert(btlecIn->scms.stopScan);
	btlecIn->scms.stopScan(btlecIn);
}


void cxa_btle_client_notify_advertRx(cxa_btle_client_t *const btlecIn, cxa_btle_advPacket_t *packetIn)
{
	cxa_assert(btlecIn);

	if( btlecIn->cbs.onAdvert != NULL ) btlecIn->cbs.onAdvert(btlecIn->cbs.userVar, packetIn);
}


void cxa_btle_client_parseAdvField(cxa_btle_advField_t *const advFieldIn, uint8_t* bytesIn)
{
	cxa_assert(advFieldIn);
	cxa_assert(bytesIn);

	// first byte is always length byte
	advFieldIn->length = *bytesIn;
	bytesIn++;

	// next is type
	advFieldIn->type = (cxa_btle_advFieldType_t)*bytesIn;
	bytesIn++;

	// the rest depends on the type
	switch( advFieldIn->type )
	{
		case CXA_BTLE_ADVFIELDTYPE_FLAGS:
			advFieldIn->asFlags.flags = *bytesIn;
			break;

		case CXA_BTLE_ADVFIELDTYPE_TXPOWER:
			advFieldIn->asTxPower.txPower_dBm = *bytesIn;
			break;

		case CXA_BTLE_ADVFIELDTYPE_MAN_DATA:
			advFieldIn->asManufacturerData.companyId = (uint16_t)*bytesIn;
			bytesIn++;
			advFieldIn->asManufacturerData.companyId |= ((uint16_t)*bytesIn) >> 8;
			bytesIn++;
			cxa_fixedByteBuffer_init_inPlace(&advFieldIn->asManufacturerData.manBytes, advFieldIn->length - 4, bytesIn, advFieldIn->length - 4);
			break;
	}
}


// ******** local function implementations ********
