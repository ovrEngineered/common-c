/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */

/**
 * @file
 * This file contains functions for writing to registers which are protected by the Configuration Change Protection (CCP)
 * mechanism. In general, these are system-critical registers which require some bit-twiddling and clock-cycle-critical
 * write period (things like clock selection, etc).
 *
 * @note This file contains functionality restricted to the CXA Atmel XMega implementation.
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * // write 0x01 to CLK.CTRL
 * cxa_xmega_ccp_writeIo((void*)&CLK.CTRL, 0x01);
 * @endcode
 */
#ifndef CXA_XMEGA_CCP_H_
#define CXA_XMEGA_CCP_H_


// ******** includes ********


// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
/**
 * @public
 * @brief Use this function to write the given byte to the provided CCP-protected register.
 * This function is implemented in assembly language to ensure proper access times after
 * triggering the CCP protection bit.
 *
 * @param[in] addr address of the target register which should be written (after being CCP unlocked)
 * @param[in] value the value to write to the target register
 */
extern void cxa_xmega_ccp_writeIo(void *addr, uint8_t value);


#endif // CXA_XMEGA_CCP_H_
