#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led.h"
#include "relay.h"
#include "uart.h"
#include "com.h"

static const char *TAG = "main";

void app_main(void)
{
    // Initialize UART, LED and Relays
    UartInit();
    LedInit();
    RelayInit();
    ComInit();

    ESP_LOGI(TAG, "Welcome to Web Relay");

    // Main loop - process commands from queue
    command_t cmd;
    while (1)
    {
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

            default:
                ESP_LOGW(TAG, "Unknown command type");
                break;
            }
        }

        // Small delay to prevent CPU spinning
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
