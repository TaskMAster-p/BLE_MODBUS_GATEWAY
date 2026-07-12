
#include "common.h"
#include "modbus.h"


int find_slave_index(uint8_t slave_id);
void update_holding_registers(sensor_packet_t *pkt);
void print_holding_registers(void);
void print_hex(const uint8_t *data,
               uint8_t len);
// int allocate_slave_slot(uint8_t slave_id);
uint16_t holding_registers[MAX_ACTIVE_DEVICES * REG_PER_DEVICE];
QueueHandle_t sensor_queue;
static uint16_t modbus_crc(const uint8_t *data,
                           uint16_t len){
    uint16_t crc = 0xFFFF;
    for(int i = 0; i < len; i++){
        crc ^= data[i];
        for(int j = 0; j < 8; j++){
            if(crc & 1)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/******************************************************
 * Process Modbus RTU Request
 *****************************************************/

static void process_modbus_request(uint8_t *rx, uint16_t len){
    if(len < 8)
    {
        ESP_LOGW(TAG,
                 "Invalid Modbus Frame");
        return;
    }

    /******************************************************
 * Verify CRC
 *****************************************************/

    uint16_t received_crc = rx[len - 2] | (rx[len - 1] << 8);

    uint16_t calculated_crc =  modbus_crc(rx, len - 2);

    if(received_crc != calculated_crc){
        ESP_LOGW(TAG1, "CRC Error");

        ESP_LOGI(TAG1, "Received CRC   : 0x%04X", received_crc);

        ESP_LOGI(TAG1,"Calculated CRC : 0x%04X",calculated_crc);

        return;
    }
    uint8_t slave_id = rx[0];
    uint8_t function = rx[1];
    uint16_t start_addr = (rx[2] << 8) | rx[3];
    uint16_t quantity = (rx[4] << 8) | rx[5];
    ESP_LOGI(TAG1, "Slave ID   : %d", slave_id);
    ESP_LOGI(TAG1, "Function Code : 0x%02X", function);
    ESP_LOGI(TAG1, "Start Address : %d", start_addr);
    ESP_LOGI(TAG1, "Quantity      : %d", quantity);

    if(function != 0x03){
        ESP_LOGW(TAG1, "Only FC03 Supported");
        return;
    }
    if(quantity < 4){
        ESP_LOGW(TAG1, "Expected 4 Registers");
        return;
    }

    /******************************************************
 * Find Holding Registers
 *****************************************************/

    int base = find_slave_index(slave_id);

    if(base < 0){
        ESP_LOGW(TAG1, "Unknown Slave");
        return;
    }

    /******************************************************
     * Build Response
     *****************************************************/

    uint8_t tx[13];
    tx[0] = slave_id;
    tx[1] = 0x03;
    tx[2] = 8;
    /* Temperature */

    tx[3] = holding_registers[base + 1] >> 8;
    tx[4] = holding_registers[base + 1];

    /* Humidity / Door */

    tx[5] = holding_registers[base + 2] >> 8;
    tx[6] = holding_registers[base + 2];

    /* Battery */

    tx[7] = holding_registers[base + 3] >> 8;
    tx[8] = holding_registers[base + 3];

    /* RSSI */

    tx[9]  = holding_registers[base + 4] >> 8;
    tx[10] = holding_registers[base + 4];

    /******************************************************
     * CRC
     *****************************************************/

    uint16_t crc = modbus_crc(tx,11);
    tx[11] = crc & 0xFF;
    tx[12] = crc >> 8;

    gpio_set_level(MODBUS_DE_PIN,1);

    int ret = uart_write_bytes(MODBUS_UART,(const char *)tx, sizeof(tx));
    ESP_LOGI(TAG1,"Bytes Sent = %d",ret);
    uart_wait_tx_done(MODBUS_UART, pdMS_TO_TICKS(100));
    gpio_set_level(MODBUS_DE_PIN,0);
    ESP_LOGI(TAG1, "Response");
    print_hex(tx, sizeof(tx));
}


void modbus_uart_init(void){
    uart_config_t uart_cfg =
    {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(
            MODBUS_UART,
            1024,
            1024,
            0,
            NULL,
            0);

    uart_param_config(
            MODBUS_UART,
            &uart_cfg);

    uart_set_pin(
            MODBUS_UART,
            MODBUS_TX_PIN,
            MODBUS_RX_PIN,
            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE);

    gpio_set_direction(
            MODBUS_DE_PIN,
            GPIO_MODE_OUTPUT);

    gpio_set_level(
            MODBUS_DE_PIN,
            0);
}
/******************************************************
 * Modbus Task
 *****************************************************/

void modbus_task(void *arg){
    // sensor_packet_t packet;

    ESP_LOGI(TAG1, "Modbus Task Started");

   while(1){
    sensor_packet_t packet;

    /**********************************************
     * Update Holding Registers
     **********************************************/

    while(xQueueReceive(sensor_queue, &packet, 0) == pdTRUE){
        update_holding_registers(&packet);
        print_holding_registers();
    }

    /**********************************************
     * UART Receive
     **********************************************/

    uint8_t rx_buffer[256];

    int len =
        uart_read_bytes(
                MODBUS_UART,
                rx_buffer,
                sizeof(rx_buffer),
                pdMS_TO_TICKS(100));
    ESP_LOGI(TAG1,
             "UART Read : %d Bytes",
             len);

    if(len > 0)
    {
        ESP_LOGI(TAG1,
                 "Received Modbus Frame (%d Bytes)",
                 len);

        print_hex(rx_buffer,
                  len);
        process_modbus_request(rx_buffer,
                       len);
    }

    vTaskDelay(pdMS_TO_TICKS(10));
}
}