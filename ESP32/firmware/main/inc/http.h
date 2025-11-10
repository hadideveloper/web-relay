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

#endif // HTTP_H

