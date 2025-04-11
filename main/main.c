#include <esp_log.h>
#include "rc522.h"
#include "driver/rc522_spi.h"
#include "rc522_picc.h"
#include "task_common.h"
#include "nvs_flash.h"
#include "string.h"
#include "nvs.h"
#include "UARTHandler.h"
#include "RIFD_Handler.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Pwm_servo.h"
#include "WifiSTA.h"
#include "Telegram.h"
#include "BLE_Handler.h"

static const char *TAG = "main";

bool card_present = false;
bool card_valid = false;
bool is_wifi_connected = false;

char g_atqa[10];
char g_sak[10];
char g_uid[20];

char g_ssid[20];
char g_password[20];

// Callback function to handle received BLE data

void app_main()
{
    initialize_uart();
    //servo_initial();

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Try to connect to WiFi first
    ESP_LOGI(TAG, "Attempting to connect to WiFi...");

    esp_err_t wifiConnect = wifi_init_sta();
    if (wifiConnect == ESP_OK) {
        is_wifi_connected = true;
        ESP_LOGI(TAG, "WiFi connected successfully");
    }
    //ble_init();
    // Wait for WiFi connection
    vTaskDelay(pdMS_TO_TICKS(5000));

    // If WiFi connection failed, start BLE
    if (!is_wifi_connected) {
        ESP_LOGI(TAG, "WiFi connection failed, starting BLE...");
       // ble_init();
       app_ble_start();
       app_ble_set_data_recv_callback(ble_data_received_callback);
    }

    // Create other tasks
    xTaskCreate(servo_task, "servo_task", 1024*4, NULL, configMAX_PRIORITIES-1, NULL);
    xTaskCreate(rfid_task, "rfid_task", 1024*5, NULL, configMAX_PRIORITIES-1, NULL);
    xTaskCreate(rx_task, "uart_rx_task", 4096, NULL, configMAX_PRIORITIES-1, NULL);
    xTaskCreate(telegram_notification_task, "telegram_task", 1024*6, NULL, configMAX_PRIORITIES-1, NULL);

    // Main loop to handle WiFi/BLE state
    while(1) {
        if (!is_wifi_connected) {
            ESP_LOGI(TAG, "WiFi connection failed, starting BLE broadcasting...");
            app_ble_start();
            vTaskDelay(pdMS_TO_TICKS(5000)); // Check every 10 seconds
        } else {
            app_ble_stop();
            vTaskDelay(pdMS_TO_TICKS(5000)); // Check every 10 seconds
        }
    }
}


