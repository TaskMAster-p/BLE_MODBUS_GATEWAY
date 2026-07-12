#include "common.h"


uint8_t datoms_count=0;

/*----------------------------------------------------------
 * Check whether MAC already belongs to a Datoms device
 *---------------------------------------------------------*/

int find_datoms_device(const uint8_t *mac){
    for(int i = 0; i < datoms_count; i++)
    {
        if(memcmp(mac,datoms_devices[i].mac,6) == 0){
            return i;
        }
    }
    return -1;
}

/*----------------------------------------------------------
 * Add newly discovered Datoms MAC
 *---------------------------------------------------------*/
void add_datoms_device(const uint8_t *mac,uint8_t slave_id){
    if(find_datoms_device(mac) >= 0){
        return;
    }
    if(datoms_count >= MAX_ACTIVE_DEVICES){
        return;
    }
    memcpy(datoms_devices[datoms_count].mac, mac, 6);
    datoms_devices[datoms_count].slave_id = slave_id;

    ESP_LOGI(TAG,"Added Datoms_%d",slave_id);
    datoms_count++;
}

/******************************************************
 * Find Slave ID in Holding Registers
 *****************************************************/

int find_slave_index(uint8_t slave_id){
    for(int i = 0; i < MAX_ACTIVE_DEVICES; i++){
        int base = i * REG_PER_DEVICE;
        if(holding_registers[base] == slave_id){
            return base;
        }
    }
    return -1;
}

/******************************************************
 * Allocate Holding Register Block
 *****************************************************/
int allocate_slave_slot(uint8_t slave_id){
    for(int i = 0; i < MAX_ACTIVE_DEVICES; i++){
        int base = i * REG_PER_DEVICE;

        /* Empty slot */
        if(holding_registers[base] == 0){
            holding_registers[base] = slave_id;
            return base;
        }
    }
    return -1;
}

/******************************************************
 * Update Holding Registers
 *****************************************************/
void update_holding_registers(sensor_packet_t *pkt){
    int base;
    base = find_slave_index(pkt->slave_id);

    /* New Slave */

    if(base == -1){
        base = allocate_slave_slot(pkt->slave_id);
        if(base == -1){
            ESP_LOGW(TAG,"Holding Register Table Full");
            return;
        }
    }
    /* Update only telemetry */
    holding_registers[base + 1] = pkt->temperature;
    holding_registers[base + 2] = pkt->humidity_or_door;
    holding_registers[base + 3] = pkt->battery_mv;
    holding_registers[base + 4] = (uint16_t)(int16_t)pkt->rssi;
}

/******************************************************
 * Print Holding Registers
 *****************************************************/

void print_holding_registers(void){
    printf("\n================ Holding Registers ================\n");
    for(int i = 0; i < MAX_ACTIVE_DEVICES; i++){
        int base = i * REG_PER_DEVICE;
        if(holding_registers[base] == 0)
            continue;

        printf("Slave %d\n", holding_registers[base]);
        printf("  Temp      : 0x%04X\n", holding_registers[base + 1]);
        printf("  Hum/Door  : 0x%04X\n",holding_registers[base + 2]);
        printf("  Battery   : 0x%04X\n",holding_registers[base + 3]);
        printf("  RSSI      : 0x%04X\n", holding_registers[base + 4]);
        printf("\n");
    }
    printf("===================================================\n\n");
}
