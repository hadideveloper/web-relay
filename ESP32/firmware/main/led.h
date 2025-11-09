#ifndef LED_H
#define LED_H

#include "driver/gpio.h"

/**
 * @brief Initialize the LED GPIO
 */
void LedInit(void);

/**
 * @brief Turn the LED ON
 */
void LedOn(void);

/**
 * @brief Turn the LED OFF
 */
void LedOff(void);

#endif // LED_H

