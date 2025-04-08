#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_crt_bundle.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_http_client.h"
#include "esp_netif.h"

#include "WifiSTA.h"
#include "esp_log.h"
#include "Global.h"
#include "RIFD_Handler.h"
#include "Telegram.h"

#define BOT_TOKEN "REMOVED"
#define CHAT_ID "6166062022"

static const char *TAG = "telegramBot";

// Biến để theo dõi trạng thái thẻ không hợp lệ
static bool invalid_card_notified = false;
extern bool card_present;
extern bool card_valid;
extern char current_uid[RC522_PICC_UID_STR_BUFFER_SIZE_MAX];

static int is_safe_char(unsigned char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '-' || c == '_' || c == '.' || c == '~';
}

// Hàm encode URL
static void url_encode(const char *str, char *encoded, size_t max_len) {
    char *p = encoded;
    const char *s = str;
    
    while (*s != '\0' && (p - encoded) < max_len - 1) {
        unsigned char c = (unsigned char)*s;
        if (is_safe_char(c)) {
            *p++ = c;
        } else if (c == ' ') {
            *p++ = '+';
        } else {
            if ((p - encoded) + 3 < max_len) {
                *p++ = '%';
                p += snprintf(p, max_len - (p - encoded), "%02X", c);
            }
        }
        s++;
    }
    *p = '\0';
}

esp_err_t send_telegram_message(const char *message) {
    char url[256];
    char encoded_message[256];
    char post_data[512];
    
    // URL encode message
    url_encode(message, encoded_message, sizeof(encoded_message));
    
    // Tạo URL cơ bản
    snprintf(url, sizeof(url), "https://api.telegram.org/bot%s/sendMessage", BOT_TOKEN);
    
    // Tạo post data với message đã được encoded
    snprintf(post_data, sizeof(post_data), "chat_id=%s&text=%s", CHAT_ID, encoded_message);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
        .disable_auto_redirect = true,
        .crt_bundle_attach = esp_crt_bundle_attach,  // Sử dụng certificate bundle
     
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }

    // Thiết lập header và post data
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    // Thử gửi tin nhắn tối đa 3 lần
    esp_err_t result = ESP_FAIL;
    for (int retry = 0; retry < 3; retry++) {
        esp_err_t err = esp_http_client_perform(client);
        
        if (err == ESP_OK) {
            int status_code = esp_http_client_get_status_code(client);
            if (status_code == 200) {
                ESP_LOGI(TAG, "Message sent successfully");
                result = ESP_OK;
                break;
            } else {
                ESP_LOGW(TAG, "HTTP request failed with status %d", status_code);
            }
        } else {
            ESP_LOGE(TAG, "Failed to send message: %s", esp_err_to_name(err));
        }
        
        if (retry < 2) {
            ESP_LOGI(TAG, "Retrying in 1 second (attempt %d/3)", retry + 1);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    esp_http_client_cleanup(client);
    return result;
}

// Task để gửi thông báo Telegram khi phát hiện thẻ không hợp lệ
void telegram_notification_task(void *arg) {
    ESP_LOGI(TAG, "Telegram notification task started");
    
    // Đợi một khoảng thời gian ngắn để đảm bảo WiFi đã kết nối     
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    // Vòng lặp chính để kiểm tra trạng thái thẻ
    while (1) {
        // Kiểm tra xem có thẻ không hợp lệ không
        if (card_present && !card_valid) {
            // Nếu chưa gửi thông báo cho thẻ này
            if (!invalid_card_notified) {
                ESP_LOGI(TAG, "Phát hiện thẻ không hợp lệ, gửi thông báo Telegram");
                
                // Tạo tin nhắn thông báo
                char message[256];
                snprintf(message, sizeof(message), "CẢNH BÁO: Phát hiện thẻ không hợp lệ!\nUID: %s", current_uid);
                
                // Gửi tin nhắn Telegram
                send_telegram_message(message);
                
                // Đánh dấu đã gửi thông báo
                invalid_card_notified = true;
            }
        } else if (!card_present) {
            // Reset trạng thái thông báo khi thẻ được lấy đi
            invalid_card_notified = false;
        }
        
        // Đợi một khoảng thời gian trước khi kiểm tra lại
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}