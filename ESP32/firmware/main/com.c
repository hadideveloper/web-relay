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
#define MAX_COMMAND_LENGTH 32

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
 * @brief Parse a command string and return the command type
 */
static command_type_t parse_command(const char *cmd_str)
{
    char cmd_copy[MAX_COMMAND_LENGTH];
    strncpy(cmd_copy, cmd_str, MAX_COMMAND_LENGTH - 1);
    cmd_copy[MAX_COMMAND_LENGTH - 1] = '\0';
    to_lowercase(cmd_copy);

    // Remove trailing whitespace and <CR> if present
    int len = strlen(cmd_copy);
    while (len > 0 && (cmd_copy[len - 1] == '\r' || cmd_copy[len - 1] == '\n' ||
                       cmd_copy[len - 1] == ' ' || cmd_copy[len - 1] == '\t'))
    {
        cmd_copy[--len] = '\0';
    }

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
                    command_type_t cmd_type = parse_command(buffer);

                    if (cmd_type != CMD_UNKNOWN)
                    {
                        // Create command structure
                        command_t cmd = {
                            .type = cmd_type};

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
