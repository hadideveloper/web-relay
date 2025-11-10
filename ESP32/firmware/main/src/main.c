#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led.h"
#include "relay.h"
#include "uart.h"
#include "com.h"
#include "wifi.h"

static const char *TAG = "main";

#define MAX_SSID_LEN 33
#define MAX_PASSWORD_LEN 65

void app_main(void)
{
    // Initialize UART, LED and Relays
    UartInit();
    LedInit();
    RelayInit();
    ComInit();

    // Initialize WiFi
    WifiInit();

    // Load SSID and password from NVS
    char ssid[MAX_SSID_LEN] = {0};
    char password[MAX_PASSWORD_LEN] = {0};
    bool has_ssid = (WifiLoadSsid(ssid, MAX_SSID_LEN) == 0);
    bool has_password = (WifiLoadPassword(password, MAX_PASSWORD_LEN) == 0);

    if (has_ssid && has_password)
    {
        ESP_LOGI(TAG, "Loaded WiFi credentials from NVS");
        WifiConnect(ssid, password);
    }
    else
    {
        ESP_LOGW(TAG, "No WiFi credentials found in NVS, using defaults");
    }

    ESP_LOGI(TAG, "Welcome to Web Relay");

    // Main loop - process commands from queue
    command_t cmd;
    bool led_state = false;
    while (1)
    {
        // Check WiFi connection status and update LED
        if (WifiIsConnected() && !led_state)
        {
            LedOn();
            led_state = true;
            ESP_LOGI(TAG, "WiFi connected - LED ON");
        }
        else if (!WifiIsConnected() && led_state)
        {
            LedOff();
            led_state = false;
            ESP_LOGI(TAG, "WiFi disconnected - LED OFF");
        }

        // Check for commands from UART (non-blocking)
        if (ComGetCommand(pdMS_TO_TICKS(100), &cmd) == pdTRUE)
        {
            // Execute the command
            switch (cmd.type)
            {
            case CMD_LED_ON:
                LedOn();
                ESP_LOGI(TAG, "Executed: LED ON");
                break;

            case CMD_LED_OFF:
                LedOff();
                ESP_LOGI(TAG, "Executed: LED OFF");
                break;

            case CMD_RELAY1_ON:
                RelayOn(1);
                ESP_LOGI(TAG, "Executed: RELAY1 ON");
                break;

            case CMD_RELAY1_OFF:
                RelayOff(1);
                ESP_LOGI(TAG, "Executed: RELAY1 OFF");
                break;

            case CMD_RELAY2_ON:
                RelayOn(2);
                ESP_LOGI(TAG, "Executed: RELAY2 ON");
                break;

            case CMD_RELAY2_OFF:
                RelayOff(2);
                ESP_LOGI(TAG, "Executed: RELAY2 OFF");
                break;

            case CMD_SSID_SET:
                if (WifiSaveSsid(cmd.param) == 0)
                {
                    ComSendResponse("OK");
                    ESP_LOGI(TAG, "SSID saved: %s", cmd.param);
                    // Reconnect with new SSID if password is also available
                    char stored_password[MAX_PASSWORD_LEN] = {0};
                    if (WifiLoadPassword(stored_password, MAX_PASSWORD_LEN) == 0)
                    {
                        WifiConnect(cmd.param, stored_password);
                    }
                }
                else
                {
                    ComSendResponse("ERROR");
                    ESP_LOGE(TAG, "Failed to save SSID");
                }
                break;

            case CMD_WIFIPASS_SET:
                if (WifiSavePassword(cmd.param) == 0)
                {
                    ComSendResponse("OK");
                    ESP_LOGI(TAG, "Password saved");
                    // Reconnect with new password if SSID is also available
                    char stored_ssid[MAX_SSID_LEN] = {0};
                    if (WifiLoadSsid(stored_ssid, MAX_SSID_LEN) == 0)
                    {
                        WifiConnect(stored_ssid, cmd.param);
                    }
                }
                else
                {
                    ComSendResponse("ERROR");
                    ESP_LOGE(TAG, "Failed to save password");
                }
                break;

            case CMD_SSID_QUERY:
            {
                char stored_ssid[MAX_SSID_LEN] = {0};
                if (WifiLoadSsid(stored_ssid, MAX_SSID_LEN) == 0)
                {
                    ComSendResponse(stored_ssid);
                }
                else
                {
                    ComSendResponse("NOT_SET");
                }
                break;
            }

            case CMD_WIFIPASS_QUERY:
            {
                char stored_password[MAX_PASSWORD_LEN] = {0};
                if (WifiLoadPassword(stored_password, MAX_PASSWORD_LEN) == 0)
                {
                    // Mask password: first 3 chars + *** + last 2 chars
                    int len = strlen(stored_password);
                    char masked[32] = {0};

                    if (len <= 3)
                    {
                        // If password is 3 chars or less, show all as ***
                        strcpy(masked, "***");
                    }
                    else if (len <= 5)
                    {
                        // If password is 4-5 chars, show first 3 + ***
                        strncpy(masked, stored_password, 3);
                        strcat(masked, "***");
                    }
                    else
                    {
                        // Show first 3 + *** + last 2
                        strncpy(masked, stored_password, 3);
                        strcat(masked, "***");
                        strncat(masked, stored_password + len - 2, 2);
                    }
                    ComSendResponse(masked);
                }
                else
                {
                    ComSendResponse("NOT_SET");
                }
                break;
            }

            default:
                ESP_LOGW(TAG, "Unknown command type");
                break;
            }
        }

        // Small delay to prevent CPU spinning
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
