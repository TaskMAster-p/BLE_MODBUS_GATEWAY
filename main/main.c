
#include "common.h"
#include "gap.h"
#include "modbus.h"


// Function Prototypes
static void on_stack_reset(int reason);
static void on_stack_sync(void);
static void nimble_host_config_init(void);
static void nimble_host_task(void *param);


// Library Prototypes
void scan_init(void);
// int gap_event(struct ble_gap_event *event, void *arg);

void modbus_uart_init(void);
// void process_modbus_request(uint8_t *rx, uint16_t len);
void modbus_task(void *arg);
// uint16_t modbus_crc(const uint8_t *buf, uint16_t len);

// extern void print_hex(const uint8_t *data, uint8_t len);

// extern int find_datoms_device(const uint8_t *mac);
// extern void add_datoms_device(const uint8_t *mac,uint8_t slave_id);
// int find_slave_index(uint8_t slave_id);
int allocate_slave_slot(uint8_t slave_id);
// void update_holding_registers(sensor_packet_t *pkt);
// void print_holding_registers(void);


static void on_stack_reset(int reason)
{
    ESP_LOGI(TAG,"Stack Reset : %d",reason);
}

// Host Sync Callback

static void on_stack_sync(void)
{
    ESP_LOGI(TAG,"Host Synced");
    scan_init();
}

// Configuring Nimble Host

static void nimble_host_config_init(void)
{
    ble_hs_cfg.reset_cb = on_stack_reset;
    ble_hs_cfg.sync_cb = on_stack_sync;
}

// Nimble Host Stack

static void nimble_host_task(void *param)
{
    ESP_LOGI(TAG,"NimBLE Host Started");
    nimble_port_run();
    vTaskDelete(NULL);
}


void app_main(void)
{
    esp_err_t ret;

    ret = nvs_flash_init();

    if(ret == ESP_ERR_NVS_NO_FREE_PAGES ||ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG,"NVS Initialized");

    // Initialize Nimble

    ret = nimble_port_init();

    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG,"NimBLE Init Failed");
        return;
    }
    ESP_LOGI(TAG,"NimBLE Initialized");
    
    sensor_queue = xQueueCreate(20,
                            sizeof(sensor_packet_t));

    if(sensor_queue == NULL)
    {
        ESP_LOGE(TAG,
                "Queue Creation Failed");

        return;
    }

    ble_svc_gap_init();

    nimble_host_config_init();
    modbus_uart_init();
    xTaskCreatePinnedToCore(
        nimble_host_task,
        "BLE Host",
        4096,
        NULL,
        5,
        NULL,
        0);

    xTaskCreatePinnedToCore(
            modbus_task,
            "Modbus",
            4096,
            NULL,
            5,
            NULL,
            1);
}
