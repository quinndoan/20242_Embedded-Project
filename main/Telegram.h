#ifndef TELEGRAM_H
#define TELEGRAM_H

#include <esp_err.h>

/**
 * @brief Gửi tin nhắn qua Telegram
 * 
 * @param message Nội dung tin nhắn cần gửi
 * @return esp_err_t ESP_OK nếu gửi thành công, mã lỗi khác nếu thất bại
 */
esp_err_t send_telegram_message(const char *message);

/**
 * @brief Task gửi thông báo Telegram khi phát hiện thẻ không hợp lệ
 * 
 * @param arg Tham số không sử dụng
 */
void telegram_notification_task(void *arg);

#endif // TELEGRAM_H

