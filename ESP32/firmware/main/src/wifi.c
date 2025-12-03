#include "wifi.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include <string.h>
#include <stdbool.h>

static const char *TAG = "wifi";

static esp_netif_t *sta_netif = NULL;
static bool wifi_connected = false;

/**
 * @brief WiFi event handler
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "WiFi station started");
            esp_wifi_connect();
            break;

        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "WiFi connected to AP");
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGW(TAG, "WiFi disconnected from AP");
            wifi_connected = false;
            esp_wifi_connect();
            break;

        default:
            break;
        }
    }
    else if (event_base == IP_EVENT)
    {
        if (event_id == IP_EVENT_STA_GOT_IP)
        {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
            wifi_connected = true;
        }
    }
}

void WifiInit(void)
{
    // Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize network interface
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default WiFi station network interface
    sta_netif = esp_netif_create_default_wifi_sta();
    if (sta_netif == NULL)
    {
        ESP_LOGE(TAG, "Failed to create WiFi station network interface");
        return;
    }

    // Initialize WiFi with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    // Set WiFi mode to station
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    ESP_LOGI(TAG, "WiFi module initialized");
}

int WifiConnect(char *ssid, char *password)
{
    if (ssid == NULL)
    {
        ESP_LOGE(TAG, "SSID cannot be NULL");
        return -1;
    }

    // Disconnect if already connected
    esp_wifi_disconnect();

    // Configure WiFi station
    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);

    if (password != NULL)
    {
        strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    }
    else
    {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }

    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    // Set WiFi configuration
    esp_err_t ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set WiFi configuration: %s", esp_err_to_name(ret));
        return -1;
    }

    // Start WiFi (ignore error if already started)
    ret = esp_wifi_start();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "Failed to start WiFi: %s", esp_err_to_name(ret));
        return -1;
    }

    // Connect to WiFi
    ret = esp_wifi_connect();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initiate WiFi connection: %s", esp_err_to_name(ret));
        return -1;
    }

    ESP_LOGI(TAG, "WiFi connection initiated to SSID: %s", ssid);
    return 0;
}

bool WifiIsConnected(void)
{
    return wifi_connected;
}

int WifiSaveSsid(const char *ssid)
{
    if (ssid == NULL)
    {
        ESP_LOGE(TAG, "SSID cannot be NULL");
        return -1;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("wifi", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return -1;
    }

    err = nvs_set_str(nvs_handle, "ssid", ssid);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error saving SSID: %s", esp_err_to_name(err));
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

    ESP_LOGI(TAG, "SSID saved to NVS");
    return 0;
}

int WifiSavePassword(const char *password)
{
    if (password == NULL)
    {
        ESP_LOGE(TAG, "Password cannot be NULL");
        return -1;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("wifi", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return -1;
    }

    err = nvs_set_str(nvs_handle, "password", password);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error saving password: %s", esp_err_to_name(err));
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

    ESP_LOGI(TAG, "Password saved to NVS");
    return 0;
}

int WifiLoadSsid(char *ssid, size_t max_len)
{
    if (ssid == NULL || max_len == 0)
    {
        ESP_LOGE(TAG, "Invalid parameters for loading SSID");
        return -1;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("wifi", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return -1;
    }

    size_t required_size = max_len;
    err = nvs_get_str(nvs_handle, "ssid", ssid, &required_size);
    nvs_close(nvs_handle);

    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGI(TAG, "SSID not found in NVS");
        return -1;
    }
    else if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error reading SSID: %s", esp_err_to_name(err));
        return -1;
    }

    ESP_LOGI(TAG, "SSID loaded from NVS: %s", ssid);
    return 0;
}

int WifiLoadPassword(char *password, size_t max_len)
{
    if (password == NULL || max_len == 0)
    {
        ESP_LOGE(TAG, "Invalid parameters for loading password");
        return -1;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("wifi", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return -1;
    }

    size_t required_size = max_len;
    err = nvs_get_str(nvs_handle, "password", password, &required_size);
    nvs_close(nvs_handle);

    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGI(TAG, "Password not found in NVS");
        return -1;
    }
    else if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error reading password: %s", esp_err_to_name(err));
        return -1;
    }

    ESP_LOGI(TAG, "Password loaded from NVS");
    return 0;
}

int WifiGetIpAddress(char* ip_str, size_t max_len)
{
    if (ip_str == NULL || max_len == 0)
    {
        ESP_LOGE(TAG, "Invalid parameters for getting IP address");
        return -1;
    }

    if (!wifi_connected || sta_netif == NULL)
    {
        return -1;
    }

    esp_netif_ip_info_t ip_info;
    esp_err_t err = esp_netif_get_ip_info(sta_netif, &ip_info);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get IP info: %s", esp_err_to_name(err));
        return -1;
    }

    snprintf(ip_str, max_len, "%d.%d.%d.%d",
             ip4_addr1(&ip_info.ip),
             ip4_addr2(&ip_info.ip),
             ip4_addr3(&ip_info.ip),
             ip4_addr4(&ip_info.ip));
    return 0;
}
