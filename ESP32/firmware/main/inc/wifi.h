#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Initialize the WiFi module
 * This initializes the WiFi stack and network interface
 */
void WifiInit(void);

/**
 * @brief Connect to a WiFi network
 * @param ssid The SSID of the WiFi network
 * @param password The password of the WiFi network
 * @return 0 on success, -1 on failure
 */
int WifiConnect(char* ssid, char* password);

/**
 * @brief Check if WiFi is connected
 * @return true if connected, false otherwise
 */
bool WifiIsConnected(void);

/**
 * @brief Save SSID to NVS
 * @param ssid The SSID to save
 * @return 0 on success, -1 on failure
 */
int WifiSaveSsid(const char* ssid);

/**
 * @brief Save WiFi password to NVS
 * @param password The password to save
 * @return 0 on success, -1 on failure
 */
int WifiSavePassword(const char* password);

/**
 * @brief Load SSID from NVS
 * @param ssid Buffer to store the SSID (must be at least 33 bytes)
 * @param max_len Maximum length of the buffer
 * @return 0 on success, -1 on failure or not found
 */
int WifiLoadSsid(char* ssid, size_t max_len);

/**
 * @brief Load WiFi password from NVS
 * @param password Buffer to store the password (must be at least 65 bytes)
 * @param max_len Maximum length of the buffer
 * @return 0 on success, -1 on failure or not found
 */
int WifiLoadPassword(char* password, size_t max_len);

/**
 * @brief Get current IP address as string
 * @param ip_str Buffer to store IP address (must be at least 16 bytes)
 * @param max_len Maximum length of the buffer
 * @return 0 on success, -1 on failure or not connected
 */
int WifiGetIpAddress(char* ip_str, size_t max_len);

#endif // WIFI_H

