#include "WifiSTA.h"
#include "esp_log.h"

static const char *TAG = "telegramBot";

void send_telegram_message(const char *message) {
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
        return;
    }

    // Thiết lập header và post data
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    // Thử gửi tin nhắn tối đa 3 lần
    for (int retry = 0; retry < 3; retry++) {
        esp_err_t err = esp_http_client_perform(client);
        
        if (err == ESP_OK) {
            int status_code = esp_http_client_get_status_code(client);
            if (status_code == 200) {
                ESP_LOGI(TAG, "Message sent successfully");
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
}
