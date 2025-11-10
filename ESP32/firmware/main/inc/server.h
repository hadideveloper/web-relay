#ifndef SERVER_H
#define SERVER_H

#include <stddef.h>

/**
 * @brief Process server response
 * Parses JSON response and controls relays accordingly
 * @param response The response string to process
 * @param response_len Length of the response string
 * @param status_code HTTP status code (200 for success)
 */
void ServerProcessResponse(const char *response, size_t response_len, int status_code);

#endif // SERVER_H

