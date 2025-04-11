#ifndef BLE_HANDLER_H
#define BLE_HANDLER_H

#include <stdint.h>
#include <inttypes.h>
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

// BLE Service UUID
#define BLE_SERVICE_UUID   0x00FF
#define BLE_CHAR_UUID      0xFF01

// BLE Characteristic for WiFi credentials
#define WIFI_SSID_CHAR_UUID    0xFF02
#define WIFI_PASS_CHAR_UUID    0xFF03

// Maximum length for BLE data
#define MAX_BLE_DATA_LEN 256

// Callback type for receiving BLE data
typedef void (*ble_data_recv_handle_t)(uint8_t *data, uint16_t length);

// Function declarations
void app_ble_send_data(uint8_t* data, uint16_t len);
void app_ble_stop(void);
void app_ble_start(void);
void app_ble_set_data_recv_callback(void *cb);
void ble_set_wifi_credentials(const char* ssid, const char* password);
void ble_data_received_callback(uint8_t *data, uint16_t length);

#endif // BLE_HANDLER_H 