/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_NETWORK_HTTPCLIENT_H_
#define CXA_NETWORK_HTTPCLIENT_H_


// ******** includes ********
#include <stdbool.h>
#include <cxa_array.h>
#include <cxa_fixedByteBuffer.h>
#include <cxa_ioStream.h>
#include <cxa_ioStream_nullablePassthrough.h>
#include <cxa_logger_header.h>
#include <cxa_network_tcpClient.h>
#include <cxa_protocolParser_crlf.h>
#include <cxa_stateMachine.h>
#include <cxa_timeDiff.h>
#include <cxa_config.h>


// ******** global macro definitions ********
#ifndef CXA_NETWORK_HTTPCLIENT_URL_MAX_LEN_BYTES
#define CXA_NETWORK_HTTPCLIENT_URL_MAX_LEN_BYTES	64
#endif

#ifndef CXA_NETWORK_HTTPCLIENT_HOSTNAME_MAX_LEN_BYTES
#define CXA_NETWORK_HTTPCLIENT_HOSTNAME_MAX_LEN_BYTES	64
#endif

#ifndef CXA_NETWORK_HTTPCLIENT_RESPONSE_LINE_BUFFERLEN_BYTES
#define CXA_NETWORK_HTTPCLIENT_HEADER_LINE_BUFFERLEN_BYTES		92
#endif


// ******** global type definitions *********
/**
 * @public
 * Forward declaration of cxa_network_httpClient_t object
 */
typedef struct cxa_network_httpClient cxa_network_httpClient_t;


/**
 * @public
 * Callback that is called periodically once a transaction is initiated and
 * a connection to the server is established. This function should be used to add headers
 * to the HTTP request. Due to the possible length of the message and buffering concerns, this
 * function streams the headers to the server in real-time via the provided ioStream. For large
 * headers, the headers should be broken into smaller chunks and sent one-at-a-time. After each
 * chunk is sent to the ioStream, return `true` from this function to allow other tasks processor
 * time. This function will continue to be called periodically until either the server closes the
 * connection or this function returns false
 *
 * @param clientIn the client performing the post operation
 * @param iosIn the ioStream which is presently connected to the server
 * @param userVarIn the previously-provided user variable
 *
 * @return true if there is more data to send and this function should be called again. False
 * 		if the send is complete
 */
typedef bool (*cxa_network_httpClient_cb_asyncGenHeaders_t)(cxa_network_httpClient_t *const clientIn,
															cxa_ioStream_t *const iosIn,
															void* userVarIn);

/**
 * @public
 * Callback that is called periodically once `cxa_network_httpClient_post_async` is initiated
 * and a connection to the server is established. This function should be used to generate the
 * body of the message. Due to the possible length of the message and buffering concerns, this
 * function streams the body to the server in real-time via the provided ioStream. For large
 * messages, the body should be broken into smaller chunks and sent one-at-a-time. After each
 * chunk is sent to the ioStream, return `true` from this function to allow other tasks processor
 * time. This function will continue to be called periodically until either the server closes the
 * connection or this function returns false
 *
 * @param clientIn the client performing the post operation
 * @param iosIn the ioStream which is presently connected to the server
 * @param userVarIn the previously-provided user variable
 *
 * @return true if there is more data to send and this function should be called again. False
 * 		if the send is complete
 */
typedef bool (*cxa_network_httpClient_cb_postAsyncGenBody_t)(cxa_network_httpClient_t *const clientIn,
															 cxa_ioStream_t *const iosIn,
															 void* userVarIn);


/**
 * @public
 * Callback that is called once `cxa_network_httpClient_post_async` is initiated. This function
 * is always the last callback per request to 'post_async'.
 *
 * @param clientIn the client performing the post operation
 * @param didCompleteSuccessfully true if the post was completed by the "genBody" callback returning
 * 			false (and all data being sent/received successfully). False if the connection was closed
 * 			before the operation could complete.
 * @param statusIn the HTTP status code (valid if 'didCompleteSuccesssfully' is true)
 * @param bodyIn the body of the response (valid if 'didCompleteSuccesssfully' is true)
 * @param bodySize_bytesIn number of bytes in the body (valid if 'didCompleteSuccesssfully' is true)
 */
typedef void (*cxa_network_httpClient_cb_onPostComplete_t)(cxa_network_httpClient_t *const clientIn,
														   bool didCompleteSuccessfully,
														   uint16_t statusIn, char *const bodyIn, size_t bodySize_bytesIn,
														   void* userVarIn);


/**
 * @private
 */
struct cxa_network_httpClient
{
	cxa_network_tcpClient_t* tcpClient;

	struct {
		cxa_network_httpClient_cb_asyncGenHeaders_t genHeaders;
		cxa_network_httpClient_cb_postAsyncGenBody_t genBody;
		cxa_network_httpClient_cb_onPostComplete_t postComplete;

		void* userVar;
	}cbs;

	char hostname[CXA_NETWORK_HTTPCLIENT_HOSTNAME_MAX_LEN_BYTES+1];
	char url[CXA_NETWORK_HTTPCLIENT_URL_MAX_LEN_BYTES+1];
	uint16_t portNum;
	bool useTls;
	uint32_t timeout_ms;
	bool keepOpen;

	cxa_fixedByteBuffer_t headerLineBuffer;
	uint8_t headerLineBuffer_raw[CXA_NETWORK_HTTPCLIENT_HEADER_LINE_BUFFERLEN_BYTES];
	cxa_protocolParser_crlf_t headerLineParser;

	cxa_ioStream_nullablePassthrough_t ios_bodyGeneration;

	cxa_timeDiff_t td_receptionTimeout;

	uint16_t responseStatusCode;
	size_t responseContentLength_bytes;

	uint8_t* responseBodyBuffer;
	size_t responseBody_currSize_bytes;
	size_t responseBody_maxSize_bytes;

	cxa_stateMachine_t stateMachine;
	cxa_logger_t logger;
};


// ******** global function prototypes ********
/**
 * @private
 */
void cxa_network_httpClient_init(cxa_network_httpClient_t *const netClientIn, int threadIdIn);


/**
 * @public
 *
 * @param responseBodyBufferIn buffer in which to store the response body, NULL if body should be discarded
 */
void cxa_network_httpClient_post_async(cxa_network_httpClient_t *const netClientIn,
								 	   char *const hostNameIn, uint16_t portNumIn, bool useTlsIn,
									   const char *const urlIn, uint32_t timeout_msIn, bool keepOpenIn,
									   cxa_network_httpClient_cb_asyncGenHeaders_t cb_genHeadersIn,
									   cxa_network_httpClient_cb_postAsyncGenBody_t cb_genBodyIn,
									   cxa_network_httpClient_cb_onPostComplete_t cb_postCompleteIn,
									   uint8_t *const responseBodyBufferIn, size_t responseBody_maxSize_bytesIn,
									   void* userVarIn);


#endif // CXA_NETWORK_HTTPCLIENT_H_
