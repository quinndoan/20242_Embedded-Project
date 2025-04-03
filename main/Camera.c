#include <stdio.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_camera.h"

#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASS "your_wifi_password"
#define TELEGRAM_BOT_TOKEN "your_bot_token"
#define TELEGRAM_CHAT_ID "your_chat_id"
#define SENSOR_PIN GPIO_NUM_34  // Cảm biến rung

static const char *TAG = "TELEGRAM_ALARM";

// Cấu hình camera ESP32-CAM
esp_camera_config_t camera_config = {
    .pin_pwdn = GPIO_NUM_32,
    .pin_reset = GPIO_NUM_NC,
    .pin_xclk = GPIO_NUM_0,
    .pin_sscb_sda = GPIO_NUM_26,
    .pin_sscb_scl = GPIO_NUM_27,
    .pin_d7 = GPIO_NUM_36,
    .pin_d6 = GPIO_NUM_39,
    .pin_d5 = GPIO_NUM_34,
    .pin_d4 = GPIO_NUM_35,
    .pin_d3 = GPIO_NUM_12,
    .pin_d2 = GPIO_NUM_13,
    .pin_d1 = GPIO_NUM_14,
    .pin_d0 = GPIO_NUM_15,
    .pin_vsync = GPIO_NUM_25,
    .pin_href = GPIO_NUM_23,
    .pin_pclk = GPIO_NUM_22,
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_QVGA,
    .jpeg_quality = 12,
    .fb_count = 2
};

// Hàm kết nối WiFi
void wifi_init() {
    ESP_LOGI(TAG, "Kết nối WiFi...");
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS
        }
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
}

// Hàm chụp ảnh từ ESP32-CAM
esp_err_t capture_image() {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Không thể chụp ảnh");
        return ESP_FAIL;
    }

    // Lưu ảnh vào bộ nhớ tạm thời (không cần phải lưu vào SD)
    FILE *file = fopen("/sdcard/image.jpg", "wb");
    if (file == NULL) {
        ESP_LOGE(TAG, "Không thể lưu ảnh vào file");
        return ESP_FAIL;
    }
    fwrite(fb->buf, 1, fb->len, file);
    fclose(file);

    // Giải phóng bộ nhớ
    esp_camera_fb_return(fb);
    return ESP_OK;
}

// Hàm gửi cảnh báo qua Telegram
void send_telegram_message(const char *message) {
    char url[512];
    snprintf(url, sizeof(url),
             "https://api.telegram.org/bot%s/sendMessage?chat_id=%s&text=%s",
             TELEGRAM_BOT_TOKEN, TELEGRAM_CHAT_ID, message);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}

// Hàm gửi hình ảnh qua Telegram
void send_telegram_photo(const char *photo_path) {
    char url[512];
    snprintf(url, sizeof(url),
             "https://api.telegram.org/bot%s/sendPhoto?chat_id=%s&photo=@%s",
             TELEGRAM_BOT_TOKEN, TELEGRAM_CHAT_ID, photo_path);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}

// Task kiểm tra cảm biến rung
void vibration_sensor_task(void *arg) {
    while (1) {
        if (gpio_get_level(SENSOR_PIN) == 1) {
            ESP_LOGW(TAG, "🚨 CẢNH BÁO! Phát hiện rung động!");

            // Gửi tin nhắn cảnh báo đến Telegram
            send_telegram_message("🚨 Cảnh báo! Có rung động bất thường!");

            // Chụp ảnh và gửi qua Telegram
            if (capture_image() == ESP_OK) {
                send_telegram_photo("/sdcard/image.jpg");
            }

            vTaskDelay(pdMS_TO_TICKS(10000)); // Chống spam
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Hàm chính
void app_main() {
    ESP_LOGI(TAG, "🚀 Khởi động hệ thống cảnh báo!");
    
    nvs_flash_init();
    wifi_init();

    // Cấu hình GPIO cảm biến rung
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << SENSOR_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Cấu hình Camera
    esp_camera_init(&camera_config);

    // Chạy task kiểm tra cảm biến rung
    xTaskCreate(vibration_sensor_task, "vibration_sensor_task", 4096, NULL, 10, NULL);
}
