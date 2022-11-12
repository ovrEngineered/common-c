/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_NVS_MANAGER_H_
#define CXA_NVS_MANAGER_H_


// ******** includes ********
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// ******** global macro definitions ********


// ******** global type definitions *********


// ******** global function prototypes ********
bool cxa_nvsManager_doesKeyExist(const char *const keyIn);

bool cxa_nvsManager_get_cString(const char *const keyIn, char *const valueOut, size_t maxOutputSize_bytes);
void cxa_nvsManager_get_cString_withDefault(const char *const keyIn, char *const valueOut, size_t maxOutputSize_bytes, char *const defaultIn);
bool cxa_nvsManager_set_cString(const char *const keyIn, char *const valueIn);

bool cxa_nvsManager_get_uint8(const char *const keyIn, uint8_t *const valueOut);
void cxa_nvsManager_get_uint8_withDefault(const char *const keyIn, uint8_t *const valueOut, uint8_t defaultIn);
bool cxa_nvsManager_set_uint8(const char *const keyIn, uint8_t valueIn);

bool cxa_nvsManager_get_uint32(const char *const keyIn, uint32_t *const valueOut);
void cxa_nvsManager_get_uint32_withDefault(const char *const keyIn, uint32_t *const valueOut, uint32_t defaultIn);
bool cxa_nvsManager_set_uint32(const char *const keyIn, uint32_t valueIn);

bool cxa_nvsManager_get_float(const char *const keyIn, float *const valueOut);
void cxa_nvsManager_get_float_withDefault(const char *const keyIn, float *const valueOut, float defaultIn);
bool cxa_nvsManager_set_float(const char *const keyIn, float valueIn);

bool cxa_nvsManager_get_blob(const char *const keyIn, uint8_t *const valueOut, size_t maxOutputSize_bytesIn, size_t *const actualOutputSize_bytesOut);
bool cxa_nvsManager_set_blob(const char *const keyIn, uint8_t *const valueIn, size_t blobSize_bytesIn);

bool cxa_nvsManager_erase(const char *const keyIn);
bool cxa_nvsManager_eraseAll(void);

bool cxa_nvsManager_commit(void);


#endif /* CXA_NVS_MANAGER_H_ */
