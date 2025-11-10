#include "http.h"
#include "wifi.h"
#include "uart.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_tls.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "http";

#define HTTP_POLL_INTERVAL_MS 2000
#define HTTP_BUFFER_SIZE 512
#define MAX_URL_LENGTH 128

static char current_url[MAX_URL_LENGTH] = {0};

/**
 * @brief HTTP event handler
 */
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        // Write data to UART (handles both chunked and non-chunked responses)
        if (evt->data_len > 0)
        {
            UartWrite((const char *)evt->data, evt->data_len);
            ESP_LOGD(TAG, "Received %d bytes, written to UART", evt->data_len);
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        // Add newline after response
        UartWrite("\r\n", 2);
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    default:
        break;
    }
    return ESP_OK;
}

/**
 * @brief Fetch the URL and write response to UART
 */
static void http_fetch_url(void)
{
    // Use current URL (should be checked before calling this function)
    if (strlen(current_url) == 0)
    {
        ESP_LOGW(TAG, "URL not set, skipping HTTP request");
        return;
    }

    const char *url_to_fetch = current_url;

    esp_http_client_config_t config = {
        .url = url_to_fetch,
        .event_handler = http_event_handler,
        .timeout_ms = 10000,
        .keep_alive_enable = false, // Disable keep-alive to avoid connection issues
    };

    // Retry up to 3 times
    int retry_count = 0;
    const int max_retries = 3;
    esp_err_t err = ESP_FAIL;

    while (retry_count < max_retries && err != ESP_OK)
    {
        if (retry_count > 0)
        {
            ESP_LOGW(TAG, "Retrying HTTP request (attempt %d/%d)...", retry_count + 1, max_retries);
            vTaskDelay(pdMS_TO_TICKS(1000)); // Wait 1 second before retry
        }

        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (client == NULL)
        {
            ESP_LOGE(TAG, "Failed to initialize HTTP client");
            retry_count++;
            continue;
        }

        // Set HTTP method to GET
        esp_http_client_set_method(client, HTTP_METHOD_GET);

        err = esp_http_client_perform(client);
        if (err == ESP_OK)
        {
            int status_code = esp_http_client_get_status_code(client);
            int content_length = esp_http_client_get_content_length(client);
            ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", status_code, content_length);
        }
        else
        {
            ESP_LOGE(TAG, "HTTP GET request failed: %s (attempt %d/%d)",
                     esp_err_to_name(err), retry_count + 1, max_retries);
        }

        esp_http_client_cleanup(client);
        retry_count++;
    }

    if (err != ESP_OK)
    {
        // Write error to UART after all retries failed
        char error_msg[64];
        snprintf(error_msg, sizeof(error_msg), "HTTP Error: %s\r\n", esp_err_to_name(err));
        UartWrite(error_msg, strlen(error_msg));
    }
}

/**
 * @brief HTTP polling task
 * Fetches the URL every 2 seconds when WiFi is connected
 */
static void http_polling_task(void *pvParameters)
{
    ESP_LOGI(TAG, "HTTP polling task started");

    // Wait a bit after task start to ensure WiFi is fully ready
    vTaskDelay(pdMS_TO_TICKS(2000));

    while (1)
    {
        // Wait for WiFi connection and check if URL is set
        if (WifiIsConnected())
        {
            // Only fetch if URL is configured
            if (strlen(current_url) > 0)
            {
                ESP_LOGD(TAG, "WiFi connected, fetching URL");
                http_fetch_url();
            }
            else
            {
                ESP_LOGD(TAG, "WiFi connected but URL not set, skipping HTTP request");
            }
        }
        else
        {
            ESP_LOGD(TAG, "WiFi not connected, waiting...");
        }

        // Wait 2 seconds before next poll
        vTaskDelay(pdMS_TO_TICKS(HTTP_POLL_INTERVAL_MS));
    }
}

void HttpInit(void)
{
    // Load URL from NVS
    char url[MAX_URL_LENGTH] = {0};
    if (HttpLoadUrl(url, MAX_URL_LENGTH) == 0)
    {
        ESP_LOGI(TAG, "Loaded URL from NVS: %s", url);
    }
    else
    {
        ESP_LOGI(TAG, "No URL found in NVS, HTTP polling will be skipped until URL is set");
        // Don't set default URL - skip HTTP requests until URL is configured
        current_url[0] = '\0';
    }

    ESP_LOGI(TAG, "HTTP client module initialized");
}

void HttpStartPolling(void)
{
    // Create the HTTP polling task
    xTaskCreate(http_polling_task, "http_polling", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "HTTP polling task created");
}

int HttpSaveUrl(const char *url)
{
    if (url == NULL)
    {
        ESP_LOGE(TAG, "URL cannot be NULL");
        return -1;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("http", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return -1;
    }

    err = nvs_set_str(nvs_handle, "url", url);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error saving URL: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return -1;
    }

    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error committing NVS: %s", esp_err_to_name(err));
        return -1;
    }

    // Update current URL
    strncpy(current_url, url, MAX_URL_LENGTH - 1);
    current_url[MAX_URL_LENGTH - 1] = '\0';

    ESP_LOGI(TAG, "URL saved to NVS: %s", url);
    return 0;
}

int HttpLoadUrl(char *url, size_t max_len)
{
    if (url == NULL || max_len == 0)
    {
        ESP_LOGE(TAG, "Invalid parameters for loading URL");
        return -1;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("http", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return -1;
    }

    size_t required_size = max_len;
    err = nvs_get_str(nvs_handle, "url", url, &required_size);
    nvs_close(nvs_handle);

    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGI(TAG, "URL not found in NVS");
        return -1;
    }
    else if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error reading URL: %s", esp_err_to_name(err));
        return -1;
    }

    // Update current URL
    strncpy(current_url, url, MAX_URL_LENGTH - 1);
    current_url[MAX_URL_LENGTH - 1] = '\0';

    ESP_LOGI(TAG, "URL loaded from NVS: %s", url);
    return 0;
}
