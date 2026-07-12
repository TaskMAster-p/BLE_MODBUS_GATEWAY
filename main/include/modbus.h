
#ifndef MODBUS_H
#define MODBUS_H

#include "driver/gpio.h"
#include "driver/uart.h"

#define MODBUS_UART        UART_NUM_1

#define MODBUS_TX_PIN      GPIO_NUM_2
#define MODBUS_RX_PIN      GPIO_NUM_4
#define MODBUS_DE_PIN      GPIO_NUM_27

#endif