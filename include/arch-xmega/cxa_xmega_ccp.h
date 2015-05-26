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
 *
 *
 * @copyright 2013-2014 opencxa.org
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
