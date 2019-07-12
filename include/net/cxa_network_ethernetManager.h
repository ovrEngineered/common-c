/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_NETWORK_ETHERNET_MANAGER_H_
#define CXA_NETWORK_ETHERNET_MANAGER_H_


// ******** includes ********
#include <stdbool.h>
#include <stdint.h>
#include <cxa_config.h>


// ******** global macro definitions ********
#ifndef CXA_NETWORK_ETHMGR_MAXNUM_LISTENERS
	#define CXA_NETWORK_ETHMGR_MAXNUM_LISTENERS		2
#endif


// ******** global type definitions *********
typedef enum
{
	CXA_NETWORK_ETHSTATE_IDLE,
	CXA_NETWORK_ETHSTATE_WAIT_LINK,
	CXA_NETWORK_ETHSTATE_WAIT_DHCP,
	CXA_NETWORK_ETHSTATE_HAS_ADDRESS_DHCP,
	CXA_NETWORK_ETHSTATE_HAS_ADDRESS_AUTOIP
}cxa_network_ethernetManager_state_t;


typedef void (*cxa_network_ethernetManager_cb_t)(void* userVarIn);


// ******** global function prototypes ********
void cxa_network_ethernetManager_init(int threadIdIn);
void cxa_network_ethernetManager_addListener(cxa_network_ethernetManager_cb_t cb_idleEnterIn,
											cxa_network_ethernetManager_cb_t cb_waitLinkIn,
											cxa_network_ethernetManager_cb_t cb_waitDhcpIn,
											cxa_network_ethernetManager_cb_t cb_hasAddress_dhcpIn,
											cxa_network_ethernetManager_cb_t cb_hasAddress_autoIpIn,
											cxa_network_ethernetManager_cb_t cb_hasAddress_staticIpIn,
											void *userVarIn);

void cxa_network_ethernetManager_start(void);
void cxa_network_ethernetManager_stop(void);

cxa_network_ethernetManager_state_t cxa_network_ethernetManager_getState(void);


#endif // CXA_NETWORK_ETHERNET_MANAGER_H_
