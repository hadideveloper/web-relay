#include "led.h"
#include "driver/gpio.h"

#define LED_GPIO GPIO_NUM_23 // GPIO23 is the built-in LED on ESP32-C3 boards

void LedInit(void)
{
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
}

void LedOn(void)
{
    gpio_set_level(LED_GPIO, 1);
}

void LedOff(void)
{
    gpio_set_level(LED_GPIO, 0);
}

