#include "common.h"
#include "gap.h"

int find_datoms_device(const uint8_t *mac);
void add_datoms_device(const uint8_t *mac, uint8_t slave_id);
void print_hex(const uint8_t *data,
               uint8_t len);
datoms_device_t datoms_devices[MAX_ACTIVE_DEVICES];


static int gap_event(struct ble_gap_event *event,void *arg){
    switch(event->type)
    {
        case BLE_GAP_EVENT_DISC:
        {
            struct ble_hs_adv_fields fields;

            memset(&fields,
                0,
                sizeof(fields));

            ble_hs_adv_parse_fields(&fields,
                                    event->disc.data,
                                    event->disc.length_data);

            const uint8_t *adv = event->disc.data;
            uint8_t len = event->disc.length_data;

            sensor_packet_t sensor;
                
            int device_index =
                find_datoms_device(event->disc.addr.val);
            memset(&sensor,
                0,
                sizeof(sensor));

            sensor.slave_id =
                datoms_devices[device_index].slave_id;

            sensor.rssi = event->disc.rssi;

    /*****************************************************
     * Check whether this advertisement contains
     * Local Name = Datoms...
     *****************************************************/

            for(int i = 0; i < len;)
            {
                uint8_t field_len = adv[i];
                if(field_len == 0) break;

                if((i + field_len) >= len) break;
                uint8_t type = adv[i + 1];

                if(type == 0x09){
                    if(field_len >= 7 && memcmp(&adv[i + 2],"Datoms",6) == 0){
                        char local_name[20] = {0};
                        memcpy(local_name,&adv[i + 2],field_len - 1);

                        char *p = strchr(local_name,'_');
                        if(p != NULL){
                            uint8_t slave_id =atoi(p + 1);
                            add_datoms_device(event->disc.addr.val,slave_id);
                        }
                        ESP_LOGI(TAG, "Datoms Name : %.*s",field_len - 1,(char *)&adv[i + 2]);
                        break;
                    }
                }

                i += field_len + 1;
            }
            if(device_index < 0){
                return 0;
            }

            ESP_LOGI(TAG,"=======================================");
            ESP_LOGI(TAG, "Datoms Advertisement");
            ESP_LOGI(TAG,
                    "MAC : %02X:%02X:%02X:%02X:%02X:%02X",
                    event->disc.addr.val[5],
                    event->disc.addr.val[4],
                    event->disc.addr.val[3],
                    event->disc.addr.val[2],
                    event->disc.addr.val[1],
                    event->disc.addr.val[0]);

            ESP_LOGI(TAG,"RSSI : %d",event->disc.rssi);
            ESP_LOGI(TAG,"Advertisement Length : %d",event->disc.length_data);
            ESP_LOGI(TAG,"Raw Advertisement");

            print_hex(event->disc.data,event->disc.length_data);

            if(event->disc.length_data < 25){
                break;
            }

            const uint8_t *pkt = event->disc.data;
            uint64_t mac =
                ((uint64_t)pkt[9]  << 40) |
                ((uint64_t)pkt[10] << 32) |
                ((uint64_t)pkt[11] << 24) |
                ((uint64_t)pkt[12] << 16) |
                ((uint64_t)pkt[13] << 8)  |
                ((uint64_t)pkt[14]);
            // if(pkt[4] == 0x08) // TSense
            // {
            //     uint16_t temp_raw = (pkt[19] << 8) | pkt[20];
            //     uint16_t voltage = (pkt[16] << 8) | pkt[17];
            //     // uint8_t battery = pkt[18];
            //     uint8_t door = pkt[23];
            //     sensor.type = SENSOR_TSENSE;
            //     sensor.temperature = temp_raw;
            //     sensor.humidity_or_door = door;
            //     sensor.battery_mv = voltage;
            // }


            // else if(pkt[4] == 0x0A) // T-one
            // {
            //     uint16_t temp_raw = (pkt[22] << 8) | pkt[23];
            //     uint16_t voltage = (pkt[16] << 8) | pkt[17];
            //     // uint8_t battery = pkt[18];
            //     uint8_t humidity =pkt[24];
            //     sensor.type = SENSOR_TONE;
            //     sensor.temperature = temp_raw;
            //     sensor.humidity_or_door = humidity;
            //     sensor.battery_mv = voltage;
            // }
            switch(mac){
                case 0xD34C99CD6631ULL:{
                    uint16_t temp_raw = (pkt[19] << 8) | pkt[20];
                    uint16_t voltage = (pkt[16] << 8) | pkt[17];
                    uint8_t door = pkt[23];
                    for(int i = 0; i < datoms_count; i++)
                    {
                        if(memcmp(datoms_devices[i].mac,
                                event->disc.addr.val,
                                6) == 0)
                        {
                            sensor.slave_id = datoms_devices[i].slave_id;
                            break;
                        }
                    };
                    sensor.temperature = temp_raw;
                    sensor.humidity_or_door = door;
                    sensor.battery_mv = voltage;
                    break;
                }
                case 0xE1069E325539ULL:{
                    uint16_t temp_raw = (pkt[22] << 8) | pkt[23];
                    uint16_t voltage = (pkt[16] << 8) | pkt[17];
                    uint8_t humidity =pkt[24];
                    for(int i = 0; i < datoms_count; i++)
                    {
                        if(memcmp(datoms_devices[i].mac,
                                event->disc.addr.val,
                                6) == 0)
                        {
                            sensor.slave_id = datoms_devices[i].slave_id;
                            break;
                        }
                    };
                    sensor.temperature = temp_raw;
                    sensor.humidity_or_door = humidity;
                    sensor.battery_mv = voltage;
                    break;
                }
            }
            if(xQueueSend(sensor_queue,&sensor,0) != pdPASS){
                    ESP_LOGW(TAG,"Queue Full");
            }

            break;
        }
        case BLE_GAP_EVENT_DISC_COMPLETE:
            ESP_LOGI(TAG,"Scan Complete");
            break;
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI(TAG, "Connection Event");
            break;
        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG,"Disconnected");
            break;
        default:
            break;
        }

    return 0;
}


void scan_init(void){
    uint8_t own_addr_type;
    int rc;
    rc = ble_hs_id_infer_auto(0,&own_addr_type);

    if(rc != 0)
    {
        ESP_LOGE(TAG,"Address Type Failed");
        return;
    }

    struct ble_gap_disc_params scan_params;

    memset(&scan_params, 0, sizeof(scan_params));

    scan_params.passive = 0;
    scan_params.itvl = 0x0010;
    scan_params.window = 0x0010;
    scan_params.filter_duplicates = 0;
    ESP_LOGI(TAG,"Starting Scan");

    rc = ble_gap_disc(own_addr_type,BLE_HS_FOREVER,&scan_params,gap_event,NULL);
    if(rc != 0)
    {
        ESP_LOGE(TAG,"Scan Failed : %d",rc);
    }
}
