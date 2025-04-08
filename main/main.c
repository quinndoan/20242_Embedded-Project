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
static const char *TAG = "main";

bool card_present = false;
bool card_valid = false;

char g_atqa[10];
char g_sak[10];
char g_uid[20];


void app_main()
{
   initialize_uart();
   //servo_initial();

   // ESP_ERROR_CHECK(init_nvs());
    esp_err_t ret = nvs_flash_init();

    // setup wifi and telegram
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    // Đợi thêm 2 giây để đảm bảo kết nối ổn định
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Create tasks
   // xTaskCreate(rfid_task, "rfid_task", 1024*5, NULL, configMAX_PRIORITIES-1 , NULL);
    xTaskCreate(servo_task, "servo_task", 1024*4, NULL, configMAX_PRIORITIES-1, NULL);
    xTaskCreate(rfid_task, "rfid_task", 1024*5, NULL, configMAX_PRIORITIES-1 , NULL);
    xTaskCreate(rx_task, "uart_rx_task", 4096, NULL, configMAX_PRIORITIES-1, NULL);

    // Create task for Telegram message sending
    xTaskCreate(telegram_notification_task, "telegram_task", 1024*6, NULL, configMAX_PRIORITIES-1, NULL);
}


