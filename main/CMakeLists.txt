idf_component_register(SRCS "Pwm_servo.c" "UARTHandler.c" "main.c" "RIFD_Handler.c" "WifiSTA.c" "Telegram.c" "BLE_Handler.c"
                    INCLUDE_DIRS "."
                    REQUIRES "esp-idf-rc522-main"
                             "nvs_flash"
                             "driver"
                             "esp_event"
                             "esp_wifi"
                             "mbedtls"
                             "esp_http_client"
                             "bt")
                                
                             