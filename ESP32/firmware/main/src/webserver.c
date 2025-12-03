#include "webserver.h"
#include "http.h"
#include "relay.h"
#include "wifi.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "driver/gpio.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "webserver";
static httpd_handle_t server_handle = NULL;

// Track relay states
static bool relay1_state = false;
static bool relay2_state = false;

/**
 * @brief Get current relay state
 */
static int get_relay_state(int relay_num)
{
    if (relay_num == 1)
    {
        return relay1_state ? 1 : 0;
    }
    else
    {
        return relay2_state ? 1 : 0;
    }
}

/**
 * @brief Generate HTML page
 */
static const char *html_page =
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<title>Web Relay Control</title>"
    "<style>"
    "body { font-family: Arial; margin: 20px; background: #f5f5f5; }"
    ".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }"
    "h1 { color: #333; }"
    ".section { margin: 20px 0; padding: 15px; background: #f9f9f9; border-radius: 5px; }"
    "input[type=\"text\"] { width: 100%; padding: 8px; margin: 5px 0; box-sizing: border-box; }"
    "button { padding: 10px 20px; margin: 5px; border: none; border-radius: 4px; cursor: pointer; font-size: 14px; }"
    ".btn-on { background: #4CAF50; color: white; }"
    ".btn-off { background: #f44336; color: white; }"
    ".btn-save { background: #2196F3; color: white; }"
    ".status { padding: 10px; margin: 10px 0; border-radius: 4px; }"
    ".status-on { background: #d4edda; color: #155724; }"
    ".status-off { background: #f8d7da; color: #721c24; }"
    "</style>"
    "</head>"
    "<body>"
    "<div class=\"container\">"
    "<h1>Web Relay Control</h1>"
    "<div class=\"section\">"
    "<p><strong>IP Address:</strong> %s</p>"
    "</div>"
    "<div class=\"section\">"
    "<h2>Set Server URL</h2>"
    "<form method=\"POST\" action=\"/seturl\">"
    "<input type=\"text\" name=\"url\" placeholder=\"https://example.com/api/relay\" value=\"%s\">"
    "<button type=\"submit\" class=\"btn-save\">Save URL</button>"
    "</form>"
    "</div>"
    "<div class=\"section\">"
    "<h2>Relay 1</h2>"
    "<div class=\"status %s\">Status: %s</div>"
    "<button onclick=\"location.href='/relay1/on'\" class=\"btn-on\">ON</button>"
    "<button onclick=\"location.href='/relay1/off'\" class=\"btn-off\">OFF</button>"
    "</div>"
    "<div class=\"section\">"
    "<h2>Relay 2</h2>"
    "<div class=\"status %s\">Status: %s</div>"
    "<button onclick=\"location.href='/relay2/on'\" class=\"btn-on\">ON</button>"
    "<button onclick=\"location.href='/relay2/off'\" class=\"btn-off\">OFF</button>"
    "</div>"
    "</div>"
    "</body>"
    "</html>";

/**
 * @brief Handler for root GET request
 */
static esp_err_t root_get_handler(httpd_req_t *req)
{
    char url[128] = {0};
    HttpLoadUrl(url, sizeof(url));
    if (strlen(url) == 0)
    {
        strcpy(url, "Not set");
    }

    char ip_str[16] = "Not connected";
    WifiGetIpAddress(ip_str, sizeof(ip_str));

    int relay1_state_val = get_relay_state(1);
    int relay2_state_val = get_relay_state(2);

    const char *relay1_status = relay1_state_val ? "ON" : "OFF";
    const char *relay2_status = relay2_state_val ? "ON" : "OFF";
    const char *relay1_class = relay1_state_val ? "status-on" : "status-off";
    const char *relay2_class = relay2_state_val ? "status-on" : "status-off";

    char html[2048];
    snprintf(html, sizeof(html), html_page,
             ip_str,
             url,
             relay1_class, relay1_status,
             relay2_class, relay2_status);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/**
 * @brief Handler for /seturl POST request
 */
static esp_err_t seturl_post_handler(httpd_req_t *req)
{
    char content[256];
    size_t recv_size = sizeof(content) - 1;

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    content[ret] = '\0';

    // Parse URL from form data (url=...)
    char *url_start = strstr(content, "url=");
    if (url_start)
    {
        url_start += 4; // Skip "url="
        // URL decode (simple version - replace + with space, %20 with space)
        char url[128] = {0};
        int i = 0, j = 0;
        while (url_start[i] && j < sizeof(url) - 1)
        {
            if (url_start[i] == '+')
            {
                url[j++] = ' ';
            }
            else if (url_start[i] == '%' && url_start[i + 1] == '2' && url_start[i + 2] == '0')
            {
                url[j++] = ' ';
                i += 2;
            }
            else if (url_start[i] == '&')
            {
                break; // End of url parameter
            }
            else
            {
                url[j++] = url_start[i];
            }
            i++;
        }
        url[j] = '\0';

        if (strlen(url) > 0)
        {
            if (HttpSaveUrl(url) == 0)
            {
                ESP_LOGI(TAG, "URL saved via web: %s", url);
                httpd_resp_set_status(req, "303 See Other");
                httpd_resp_set_hdr(req, "Location", "/");
                httpd_resp_send(req, NULL, 0);
                return ESP_OK;
            }
        }
    }

    httpd_resp_set_status(req, "400 Bad Request");
    httpd_resp_send(req, "Invalid URL", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/**
 * @brief Handler for relay control
 */
static esp_err_t relay_handler(httpd_req_t *req)
{
    const char *uri = req->uri;

    if (strstr(uri, "/relay1/on"))
    {
        RelayOn(1);
        relay1_state = true;
        ESP_LOGI(TAG, "Relay 1 turned ON via web");
    }
    else if (strstr(uri, "/relay1/off"))
    {
        RelayOff(1);
        relay1_state = false;
        ESP_LOGI(TAG, "Relay 1 turned OFF via web");
    }
    else if (strstr(uri, "/relay2/on"))
    {
        RelayOn(2);
        relay2_state = true;
        ESP_LOGI(TAG, "Relay 2 turned ON via web");
    }
    else if (strstr(uri, "/relay2/off"))
    {
        RelayOff(2);
        relay2_state = false;
        ESP_LOGI(TAG, "Relay 2 turned OFF via web");
    }

    // Redirect back to home
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

void WebserverInit(void)
{
    // Initialize relay states from GPIO
    relay1_state = (gpio_get_level(GPIO_NUM_16) == 1);
    relay2_state = (gpio_get_level(GPIO_NUM_17) == 1);

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.stack_size = 8192; // Increase stack size to prevent overflow

    ESP_LOGI(TAG, "Starting web server on port: '%d'", config.server_port);

    if (httpd_start(&server_handle, &config) == ESP_OK)
    {
        // Register URI handlers
        httpd_uri_t root = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_get_handler,
        };
        httpd_register_uri_handler(server_handle, &root);

        httpd_uri_t seturl = {
            .uri = "/seturl",
            .method = HTTP_POST,
            .handler = seturl_post_handler,
        };
        httpd_register_uri_handler(server_handle, &seturl);

        httpd_uri_t relay1_on = {
            .uri = "/relay1/on",
            .method = HTTP_GET,
            .handler = relay_handler,
        };
        httpd_register_uri_handler(server_handle, &relay1_on);

        httpd_uri_t relay1_off = {
            .uri = "/relay1/off",
            .method = HTTP_GET,
            .handler = relay_handler,
        };
        httpd_register_uri_handler(server_handle, &relay1_off);

        httpd_uri_t relay2_on = {
            .uri = "/relay2/on",
            .method = HTTP_GET,
            .handler = relay_handler,
        };
        httpd_register_uri_handler(server_handle, &relay2_on);

        httpd_uri_t relay2_off = {
            .uri = "/relay2/off",
            .method = HTTP_GET,
            .handler = relay_handler,
        };
        httpd_register_uri_handler(server_handle, &relay2_off);

        ESP_LOGI(TAG, "Web server started successfully");
    }
    else
    {
        ESP_LOGE(TAG, "Error starting web server!");
    }
}

void WebserverStart(void)
{
    // Already started in Init
}
