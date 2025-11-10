#include "uart.h"
#include "driver/uart.h"
#include <string.h>

#define UART_NUM UART_NUM_0
#define BUF_SIZE 1024

void UartInit(void)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
}

int UartWrite(const char *data, size_t length)
{
    if (data == NULL)
    {
        return -1;
    }
    
    return uart_write_bytes(UART_NUM, data, length);
}

int UartReadBytes(uint8_t *data, size_t length, TickType_t timeout_ms)
{
    if (data == NULL)
    {
        return -1;
    }
    
    return uart_read_bytes(UART_NUM, data, length, timeout_ms);
}

int UartReadByte(TickType_t timeout_ms)
{
    uint8_t byte;
    int result = uart_read_bytes(UART_NUM, &byte, 1, timeout_ms);
    if (result == 1)
    {
        return (int)byte;
    }
    return -1;
}

