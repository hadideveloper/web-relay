#include "server.h"
#include "relay.h"
#include "http.h"
#include "esp_log.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "server";

/**
 * @brief Structure for relay timer task parameters
 */
typedef struct
{
    int relay_num;
    int duration_ms;
} relay_timer_params_t;

/**
 * @brief Task to handle timed relay operations
 * Turns off relay after specified duration
 */
static void relay_timer_task(void *pvParameters)
{
    relay_timer_params_t *params = (relay_timer_params_t *)pvParameters;

    if (params == NULL)
    {
        vTaskDelete(NULL);
        return;
    }

    int relay_num = params->relay_num;
    int duration_ms = params->duration_ms;

    // Wait for the specified duration
    vTaskDelay(pdMS_TO_TICKS(duration_ms));

    // Turn off the relay
    RelayOff(relay_num);
    ESP_LOGI(TAG, "Relay %d auto-turned OFF after %d ms", relay_num, duration_ms);

    // Free the parameters structure
    free(params);
    vTaskDelete(NULL);
}

/**
 * @brief Process a single relay command from JSON
 */
static void process_relay_command(cJSON *relay_obj, int relay_num)
{
    if (relay_obj == NULL || !cJSON_IsObject(relay_obj))
    {
        return;
    }

    cJSON *state = cJSON_GetObjectItem(relay_obj, "state");
    cJSON *duration = cJSON_GetObjectItem(relay_obj, "duration");

    if (state == NULL || !cJSON_IsNumber(state))
    {
        return;
    }

    int relay_state = state->valueint;
    int duration_ms = 0;

    if (duration != NULL && cJSON_IsNumber(duration))
    {
        duration_ms = duration->valueint;
    }

    if (relay_state == 1)
    {
        ESP_LOGI(TAG, "Turning ON relay %d", relay_num);
        RelayOn(relay_num);
        ESP_LOGI(TAG, "Relay %d turned ON", relay_num);

        // If duration is specified and > 0, set up auto-off timer
        if (duration_ms > 0)
        {
            // Allocate parameters structure
            relay_timer_params_t *params = malloc(sizeof(relay_timer_params_t));
            if (params != NULL)
            {
                params->relay_num = relay_num;
                params->duration_ms = duration_ms;
                char task_name[16];
                snprintf(task_name, sizeof(task_name), "relay%d_timer", relay_num);
                xTaskCreate(relay_timer_task, task_name, 2048, params, 5, NULL);
                ESP_LOGI(TAG, "Relay %d will auto-turn OFF after %d ms", relay_num, duration_ms);
            }
        }
    }
    else if (relay_state == 0)
    {
        ESP_LOGI(TAG, "Turning OFF relay %d", relay_num);
        RelayOff(relay_num);
        ESP_LOGI(TAG, "Relay %d turned OFF", relay_num);
    }
    else
    {
        ESP_LOGW(TAG, "Invalid relay state value: %d (expected 0 or 1)", relay_state);
    }
}

void ServerProcessResponse(const char *response, size_t response_len, int status_code)
{
    // Only process if status is 200
    if (status_code != 200 || response == NULL || response_len == 0)
    {
        return;
    }

    // Ensure response is null-terminated (create a copy if needed)
    char *response_copy = malloc(response_len + 1);
    if (response_copy == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for response");
        return;
    }

    memcpy(response_copy, response, response_len);
    response_copy[response_len] = '\0';

    // Trim whitespace from response
    char *trimmed = response_copy;
    while (*trimmed == ' ' || *trimmed == '\t' || *trimmed == '\r' || *trimmed == '\n')
    {
        trimmed++;
    }
    int len = strlen(trimmed);
    while (len > 0 && (trimmed[len - 1] == ' ' || trimmed[len - 1] == '\t' ||
                       trimmed[len - 1] == '\r' || trimmed[len - 1] == '\n'))
    {
        trimmed[--len] = '\0';
    }

    if (len == 0)
    {
        free(response_copy);
        return;
    }

    // Try to parse as JSON
    cJSON *json = cJSON_Parse(trimmed);
    if (json != NULL)
    {
        ESP_LOGI(TAG, "JSON parsed successfully");

        // Check if this is an empty JSON object (no commands)
        // An empty object {} will have no children
        if (json->child == NULL)
        {
            ESP_LOGD(TAG, "Empty JSON object received (no commands)");
            cJSON_Delete(json);
            free(response_copy);
            return;
        }

        // Extract command_id first
        cJSON *command_id = cJSON_GetObjectItem(json, "command_id");
        if (command_id != NULL && cJSON_IsString(command_id))
        {
            ESP_LOGI(TAG, "Command ID: %s", command_id->valuestring);
        }

        // Process relay1
        cJSON *relay1 = cJSON_GetObjectItem(json, "relay1");
        if (relay1 != NULL)
        {
            ESP_LOGI(TAG, "Processing relay1 command");
            process_relay_command(relay1, 1);
        }

        // Process relay2
        cJSON *relay2 = cJSON_GetObjectItem(json, "relay2");
        if (relay2 != NULL)
        {
            ESP_LOGI(TAG, "Processing relay2 command");
            process_relay_command(relay2, 2);
        }

        // Send ACK via POST if command_id is present
        if (command_id != NULL && cJSON_IsString(command_id))
        {
            // Create ACK JSON with command_id
            cJSON *ack_json = cJSON_CreateObject();
            cJSON_AddStringToObject(ack_json, "command_id", command_id->valuestring);
            cJSON_AddStringToObject(ack_json, "status", "received");

            char *ack_str = cJSON_PrintUnformatted(ack_json);
            if (ack_str)
            {
                ESP_LOGI(TAG, "Sending ACK for command_id: %s", command_id->valuestring);
                HttpPostJson(ack_str);
                free(ack_str);
            }
            cJSON_Delete(ack_json);
        }

        cJSON_Delete(json);
    }
    else
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        ESP_LOGW(TAG, "Failed to parse JSON response: %s", error_ptr ? error_ptr : "unknown error");

        // Fallback: try simple string parsing for backward compatibility
        if (strcmp(trimmed, "0") == 0)
        {
            RelayOff(1);
            ESP_LOGI(TAG, "Response is '0', turning OFF relay 1");
        }
        else if (strcmp(trimmed, "1") == 0)
        {
            RelayOn(1);
            ESP_LOGI(TAG, "Response is '1', turning ON relay 1");
        }
    }

    free(response_copy);
}
