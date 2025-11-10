#ifndef RELAY_H
#define RELAY_H

#include "driver/gpio.h"

/**
 * @brief Initialize the relay GPIOs
 */
void RelayInit(void);

/**
 * @brief Turn a relay ON
 * @param relayNumber The relay number (1 or 2)
 */
void RelayOn(int relayNumber);

/**
 * @brief Turn a relay OFF
 * @param relayNumber The relay number (1 or 2)
 */
void RelayOff(int relayNumber);

#endif // RELAY_H

