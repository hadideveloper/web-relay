#ifndef COM_H
#define COM_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/**
 * @brief Command types
 */
typedef enum
{
    CMD_LED_ON,
    CMD_LED_OFF,
    CMD_RELAY1_ON,
    CMD_RELAY1_OFF,
    CMD_RELAY2_ON,
    CMD_RELAY2_OFF,
    CMD_UNKNOWN
} command_type_t;

/**
 * @brief Command structure
 */
typedef struct
{
    command_type_t type;
} command_t;

/**
 * @brief Initialize the communication module
 * This creates the command queue and starts the UART reading task
 */
void ComInit(void);

/**
 * @brief Get a command from the queue
 * @param timeout_ms Timeout in milliseconds (0 = no timeout, portMAX_DELAY = wait forever)
 * @param cmd Pointer to store the command
 * @return pdTRUE if a command was received, pdFALSE on timeout
 */
BaseType_t ComGetCommand(TickType_t timeout_ms, command_t *cmd);

#endif // COM_H
