#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

/**
 * @brief Initialize the HTTP client module
 */
void HttpInit(void);

/**
 * @brief Start the HTTP polling task
 * This task will fetch the URL every 2 seconds when WiFi is connected
 */
void HttpStartPolling(void);

/**
 * @brief Save URL to NVS
 * @param url The URL to save
 * @return 0 on success, -1 on failure
 */
int HttpSaveUrl(const char* url);

/**
 * @brief Load URL from NVS
 * @param url Buffer to store the URL (must be at least 128 bytes)
 * @param max_len Maximum length of the buffer
 * @return 0 on success, -1 on failure or not found
 */
int HttpLoadUrl(char* url, size_t max_len);

/**
 * @brief Send POST request with JSON payload to the configured URL
 * Uses the same endpoint as GET requests (same URL, different HTTP method)
 * @param json_payload The JSON string to send
 * @return 0 on success, -1 on failure
 */
int HttpPostJson(const char* json_payload);

#endif // HTTP_H

