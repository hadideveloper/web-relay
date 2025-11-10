#include "com.h"
#include "uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include <ctype.h>
#include "esp_log.h"

static const char *TAG = "com";

#define COMMAND_QUEUE_SIZE 10

static QueueHandle_t command_queue = NULL;

/**
 * @brief Convert string to lowercase (in-place)
 */
static void to_lowercase(char *str)
{
    for (int i = 0; str[i]; i++)
    {
        str[i] = tolower((unsigned char)str[i]);
    }
}

/**
 * @brief Parse a command string and extract command type and parameter
 */
static command_type_t parse_command(const char *cmd_str, char *param_out)
{
    char cmd_copy[MAX_COMMAND_LENGTH];
    strncpy(cmd_copy, cmd_str, MAX_COMMAND_LENGTH - 1);
    cmd_copy[MAX_COMMAND_LENGTH - 1] = '\0';

    // Remove trailing whitespace and <CR> if present
    int len = strlen(cmd_copy);
    while (len > 0 && (cmd_copy[len - 1] == '\r' || cmd_copy[len - 1] == '\n' ||
                       cmd_copy[len - 1] == ' ' || cmd_copy[len - 1] == '\t'))
    {
        cmd_copy[--len] = '\0';
    }

    // Check for SSID= command
    if (strncmp(cmd_copy, "SSID=", 5) == 0)
    {
        if (param_out != NULL && len > 5)
        {
            strncpy(param_out, cmd_copy + 5, MAX_PARAM_LENGTH - 1);
            param_out[MAX_PARAM_LENGTH - 1] = '\0';
        }
        return CMD_SSID_SET;
    }

    // Check for WIFIPASS= command
    if (strncmp(cmd_copy, "WIFIPASS=", 9) == 0)
    {
        if (param_out != NULL && len > 9)
        {
            strncpy(param_out, cmd_copy + 9, MAX_PARAM_LENGTH - 1);
            param_out[MAX_PARAM_LENGTH - 1] = '\0';
        }
        return CMD_WIFIPASS_SET;
    }

    // Check for SSID? query
    if (strcmp(cmd_copy, "SSID?") == 0)
    {
        return CMD_SSID_QUERY;
    }

    // Check for WIFIPASS? query
    if (strcmp(cmd_copy, "WIFIPASS?") == 0)
    {
        return CMD_WIFIPASS_QUERY;
    }

    // Check for URL= command
    if (strncmp(cmd_copy, "URL=", 4) == 0)
    {
        if (param_out != NULL && len > 4)
        {
            strncpy(param_out, cmd_copy + 4, MAX_PARAM_LENGTH - 1);
            param_out[MAX_PARAM_LENGTH - 1] = '\0';
        }
        return CMD_URL_SET;
    }

    // Check for URL? query
    if (strcmp(cmd_copy, "URL?") == 0)
    {
        return CMD_URL_QUERY;
    }

    // Convert to lowercase for other commands
    to_lowercase(cmd_copy);

    if (strcmp(cmd_copy, "led on") == 0)
    {
        return CMD_LED_ON;
    }
    else if (strcmp(cmd_copy, "led off") == 0)
    {
        return CMD_LED_OFF;
    }
    else if (strcmp(cmd_copy, "relay1 on") == 0)
    {
        return CMD_RELAY1_ON;
    }
    else if (strcmp(cmd_copy, "relay1 off") == 0)
    {
        return CMD_RELAY1_OFF;
    }
    else if (strcmp(cmd_copy, "relay2 on") == 0)
    {
        return CMD_RELAY2_ON;
    }
    else if (strcmp(cmd_copy, "relay2 off") == 0)
    {
        return CMD_RELAY2_OFF;
    }
    else
    {
        return CMD_UNKNOWN;
    }
}

/**
 * @brief UART command reading task
 * This task continuously reads from UART, parses commands ending with <CR>,
 * and adds them to the command queue
 */
static void com_task(void *pvParameters)
{
    char buffer[MAX_COMMAND_LENGTH];
    int buffer_index = 0;
    int byte;

    ESP_LOGI(TAG, "COM task started");

    while (1)
    {
        // Read a byte from UART with a timeout
        byte = UartReadByte(pdMS_TO_TICKS(100));

        if (byte >= 0)
        {
            // Check for <CR> (carriage return, ASCII 13) or <LF> (line feed, ASCII 10)
            if (byte == '\r' || byte == '\n')
            {
                if (buffer_index > 0)
                {
                    // Null terminate the command
                    buffer[buffer_index] = '\0';

                    // Parse the command
                    command_t cmd = {0};
                    cmd.type = parse_command(buffer, cmd.param);

                    if (cmd.type != CMD_UNKNOWN)
                    {
                        // Add to queue (non-blocking)
                        if (xQueueSend(command_queue, &cmd, 0) != pdTRUE)
                        {
                            ESP_LOGW(TAG, "Command queue full, dropping command: %s", buffer);
                        }
                        else
                        {
                            ESP_LOGI(TAG, "Received command: %s", buffer);
                        }
                    }
                    else
                    {
                        ESP_LOGW(TAG, "Unknown command: %s", buffer);
                    }

                    // Reset buffer
                    buffer_index = 0;
                }
            }
            else if (buffer_index < (MAX_COMMAND_LENGTH - 1))
            {
                // Add character to buffer
                buffer[buffer_index++] = (char)byte;
            }
            else
            {
                // Buffer overflow, reset
                ESP_LOGW(TAG, "Command buffer overflow, resetting");
                buffer_index = 0;
            }
        }
    }
}

void ComInit(void)
{
    // Create command queue
    command_queue = xQueueCreate(COMMAND_QUEUE_SIZE, sizeof(command_t));
    if (command_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create command queue");
        return;
    }

    // Create the UART reading task
    xTaskCreate(com_task, "com_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "COM module initialized");
}

BaseType_t ComGetCommand(TickType_t timeout_ms, command_t *cmd)
{
    if (cmd == NULL || command_queue == NULL)
    {
        return pdFALSE;
    }

    return xQueueReceive(command_queue, cmd, timeout_ms);
}

void ComSendResponse(const char *response)
{
    if (response == NULL)
    {
        return;
    }

    size_t len = strlen(response);
    UartWrite(response, len);
    UartWrite("\r\n", 2);  // Add CR+LF for line ending
}
