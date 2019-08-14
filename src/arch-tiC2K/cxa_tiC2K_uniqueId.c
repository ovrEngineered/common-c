/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_uniqueId.h"


// ******** includes ********
#include <stdio.h>
//ESP32
//#include <esp_wifi.h>
//#include <esp_eth.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void init(void);


// ********  local variable declarations *********
static bool isInit = false;
static uint8_t id_bytes[6];
static char id_str[18];


// ******** global function implementations ********
void cxa_uniqueId_getBytes(uint8_t** bytesOut, size_t* numBytesOut)
{
//	if( !isInit ) init();
//
//	if( bytesOut ) *bytesOut = (uint8_t*)&id_bytes;
//	if( numBytesOut ) *numBytesOut = sizeof(id_bytes);
    //TODO: Need TIC2K Implementation
}


//char* cxa_uniqueId_getHexString(void)
//{
//	if( !isInit ) init();
//
//	return id_str;
//}


// ******** local function implementations ********
//static void init(void)
//{
//	if( esp_wifi_get_mac(WIFI_IF_STA, id_bytes) != ESP_OK )
//	{
//		esp_eth_get_mac(id_bytes);
//	}
//
//	sprintf(id_str, "%02X:%02X:%02X:%02X:%02X:%02X", id_bytes[0], id_bytes[1], id_bytes[2], id_bytes[3], id_bytes[4], id_bytes[5]);
//	id_str[sizeof(id_str)-1] = 0;
//
//	isInit = true;
//}
