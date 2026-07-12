
#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "nvs_flash.h"
#include "esp_log.h"



#define TAG "BLE_SCANNER"
#define TAG1 "MODBUS"
#define MAX_ACTIVE_DEVICES    20
#define REG_PER_DEVICE          5



extern QueueHandle_t sensor_queue;

extern uint16_t holding_registers[MAX_ACTIVE_DEVICES * REG_PER_DEVICE];



typedef struct
{
    uint8_t mac[6];
    uint8_t slave_id;

} datoms_device_t;
extern datoms_device_t datoms_devices[MAX_ACTIVE_DEVICES];
extern uint8_t datoms_count;
typedef struct
{
    uint8_t slave_id;
    uint16_t temperature;
    uint16_t humidity_or_door;
    uint16_t battery_mv;
    int8_t rssi;

} sensor_packet_t;

#endif

