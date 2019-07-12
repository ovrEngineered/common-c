/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_IOSTREAM_H_
#define CXA_IOSTREAM_H_


// ******** includes ********
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include <cxa_fixedByteBuffer.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the cxa_ioStream_t object
 */
typedef struct cxa_ioStream cxa_ioStream_t;


/**
 * Encapsulates the return value of a read operation
 */
typedef enum
{
	CXA_IOSTREAM_READSTAT_NODATA,		///< No data was returned by the read, but no error occurred
	CXA_IOSTREAM_READSTAT_GOTDATA,		///< Data was returned by the read
	CXA_IOSTREAM_READSTAT_ERROR 		///< Error occurred during the read
}cxa_ioStream_readStatus_t;


/**
 * @public
 * @brief Read a byte from the ioStream.
 *
 * @param[out] byteOut pointer to a location at which to store the received byte
 * @param[in] userVarIn pointer to the user-supplied variable passed to
 * 		::cxa_ioStream_bind
 *
 * @return the return status of the read
 */
typedef cxa_ioStream_readStatus_t (*cxa_ioStream_cb_readByte_t)(uint8_t *const byteOut, void *const userVarIn);


/**
 * @public
 * @brief Write bytes to the ioStream.
 *
 * @param[in] buffIn pointer to a memory location which contains the bytes
 * 		to write
 * @param[in] bufferSizeIn the number of bytes from buffIn to write.
 * 		Will always be <= SSIZE_MAX.
 * @param[in] userVarIn pointer to the user-supplied variable passed to
 * 		::cxa_ioStream_bind
 *
  * @return true if all bytes were sent / queued to be sent, false if there
 * 		was an error with the underlying ioStream. If false, number of bytes
 * 		queued or sent is undetermined.
 */
typedef bool (*cxa_ioStream_cb_writeBytes_t)(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn);


struct cxa_ioStream
{
	cxa_ioStream_cb_readByte_t readCb;
	cxa_ioStream_cb_writeBytes_t writeCb;

	void *userVar;
};


// ******** global function prototypes ********
void cxa_ioStream_init(cxa_ioStream_t *const ioStreamIn);

void cxa_ioStream_bind(cxa_ioStream_t *const ioStreamIn, cxa_ioStream_cb_readByte_t readCbIn, cxa_ioStream_cb_writeBytes_t writeCbIn, void *const userVarIn);
void cxa_ioStream_unbind(cxa_ioStream_t *const ioStreamIn);
bool cxa_ioStream_isBound(cxa_ioStream_t *const ioStreamIn);

cxa_ioStream_readStatus_t cxa_ioStream_readByte(cxa_ioStream_t *const ioStreamIn, uint8_t *const byteOut);
bool cxa_ioStream_waitForCharSequence_withTimeout(cxa_ioStream_t *const ioStreamIn, const char* targetSeqIn, uint32_t timeout_msIn);

void cxa_ioStream_clearReadBuffer(cxa_ioStream_t *const ioStreamIn);

bool cxa_ioStream_writeByte(cxa_ioStream_t *const ioStreamIn, uint8_t byteIn);
bool cxa_ioStream_writeBytes(cxa_ioStream_t *const ioStreamIn, void* buffIn, size_t bufferSize_bytesIn);
bool cxa_ioStream_writeFixedByteBuffer(cxa_ioStream_t *const ioStreamIn, cxa_fixedByteBuffer_t *const fbbIn);
bool cxa_ioStream_writeString(cxa_ioStream_t *const ioStreamIn, const char* stringIn);
bool cxa_ioStream_writeLine(cxa_ioStream_t *const ioStreamIn, const char* stringIn);
bool cxa_ioStream_writeFormattedString(cxa_ioStream_t *const ioStreamIn, const char* formatIn, ...);
bool cxa_ioStream_writeFormattedLine(cxa_ioStream_t *const ioStreamIn, const char* formatIn, ...);

bool cxa_ioStream_vWriteString(cxa_ioStream_t *const ioStreamIn,
								const char* formatIn, va_list argsIn,
								bool truncateIfTooLargeIn, const char* truncateStringIn);

#endif // CXA_IOSTREAM_H_
