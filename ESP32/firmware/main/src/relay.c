#include "relay.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define RELAY_1_GPIO GPIO_NUM_16
#define RELAY_2_GPIO GPIO_NUM_17

static const char *TAG = "relay";

void RelayInit(void)
{
    gpio_reset_pin(RELAY_1_GPIO);
    gpio_reset_pin(RELAY_2_GPIO);
    gpio_set_direction(RELAY_1_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(RELAY_2_GPIO, GPIO_MODE_OUTPUT);
}

void RelayOn(int relayNumber)
{
    gpio_num_t gpio;
    
    if (relayNumber == 1)
    {
        gpio = RELAY_1_GPIO;
    }
    else if (relayNumber == 2)
    {
        gpio = RELAY_2_GPIO;
    }
    else
    {
        ESP_LOGE(TAG, "Invalid relay number: %d", relayNumber);
        return;
    }
    
    gpio_set_level(gpio, 1);
}

void RelayOff(int relayNumber)
{
    gpio_num_t gpio;
    
    if (relayNumber == 1)
    {
        gpio = RELAY_1_GPIO;
    }
    else if (relayNumber == 2)
    {
        gpio = RELAY_2_GPIO;
    }
    else
    {
        ESP_LOGE(TAG, "Invalid relay number: %d", relayNumber);
        return;
    }
    
    gpio_set_level(gpio, 0);
}

