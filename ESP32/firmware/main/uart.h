#ifndef UART_H
#define UART_H

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief Initialize the UART
 */
void UartInit(void);

/**
 * @brief Write data to UART
 * @param data Pointer to the data buffer to write
 * @param length Length of the data to write
 * @return Number of bytes written, or -1 on error
 */
int UartWrite(const char *data, size_t length);

/**
 * @brief Read bytes from UART
 * @param data Pointer to the data buffer to store read data
 * @param length Maximum length of data to read
 * @param timeout_ms Timeout in milliseconds (0 = no timeout, portMAX_DELAY = wait forever)
 * @return Number of bytes read, or -1 on error
 */
int UartReadBytes(uint8_t *data, size_t length, TickType_t timeout_ms);

/**
 * @brief Read a single byte from UART
 * @param timeout_ms Timeout in milliseconds (0 = no timeout, portMAX_DELAY = wait forever)
 * @return The byte read, or -1 on error/timeout
 */
int UartReadByte(TickType_t timeout_ms);

#endif // UART_H

