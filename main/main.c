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

static const char *TAG = "rc522_reading_card";

bool card_present = false;
bool card_valid = false;

char g_atqa[10];
char g_sak[10];
char g_uid[20];


void app_main()
{
   initialize_uart();
   //servo_initial();

    ESP_ERROR_CHECK(init_nvs());
    
    // Create tasks
   // xTaskCreate(rfid_task, "rfid_task", 1024*5, NULL, configMAX_PRIORITIES-1 , NULL);
    xTaskCreate(servo_task, "servo_task", 1024*4, NULL, configMAX_PRIORITIES-1, NULL);
    xTaskCreate(rfid_task, "rfid_task", 1024*5, NULL, configMAX_PRIORITIES-1 , NULL);
    xTaskCreate(rx_task, "uart_rx_task", 4096, NULL, configMAX_PRIORITIES-1, NULL);

}


