/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/mcpwm_prelude.h"
#include "Global.h"
#include "Pwm_servo.h"
#include "RIFD_Handler.h"
static const char *TAG = "PWM_SERVO";
extern bool card_present;
extern bool card_valid;

// Please consult the datasheet of your servo before changing the following parameters
#define SERVO_MIN_PULSEWIDTH_US 500  // Minimum pulse width in microsecond
#define SERVO_MAX_PULSEWIDTH_US 2500  // Maximum pulse width in microsecond
#define SERVO_MIN_DEGREE        -90   // Minimum angle
#define SERVO_MAX_DEGREE        90    // Maximum angle

#define SERVO_PULSE_GPIO             2        // GPIO connects to the PWM signal line
#define SERVO_TIMEBASE_RESOLUTION_HZ 1000000  // 1MHz, 1us per tick
#define SERVO_TIMEBASE_PERIOD        20000    // 20000 ticks, 20ms

mcpwm_timer_handle_t timer = NULL;
    mcpwm_timer_config_t timer_config = {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = SERVO_TIMEBASE_RESOLUTION_HZ,
        .period_ticks = SERVO_TIMEBASE_PERIOD,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };

mcpwm_oper_handle_t oper = NULL;
mcpwm_operator_config_t operator_config = {
    .group_id = 0, // operator must be in the same group to the timer
};

mcpwm_cmpr_handle_t comparator = NULL;
mcpwm_comparator_config_t comparator_config = {
    .flags.update_cmp_on_tez = true,
};

mcpwm_gen_handle_t generator = NULL;
mcpwm_generator_config_t generator_config = {
    .gen_gpio_num = SERVO_PULSE_GPIO,
};

static inline uint32_t angle_to_compare(int angle)
{
    return (angle - SERVO_MIN_DEGREE) * (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) / (SERVO_MAX_DEGREE - SERVO_MIN_DEGREE) + SERVO_MIN_PULSEWIDTH_US;
}

void servo_initial(void)
{
    ESP_LOGI(TAG, "Create timer and operator");

    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &oper));

    ESP_LOGI(TAG, "Connect timer and operator");
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));

    ESP_LOGI(TAG, "Create comparator and generator from the operator");
    // mcpwm_cmpr_handle_t comparator = NULL;
    // mcpwm_comparator_config_t comparator_config = {
    //     .flags.update_cmp_on_tez = true,
    // };
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &comparator_config, &comparator));

    // mcpwm_gen_handle_t generator = NULL;
    // mcpwm_generator_config_t generator_config = {
    //     .gen_gpio_num = SERVO_PULSE_GPIO,
    // };
    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &generator_config, &generator));

    // set the initial compare value, so that the servo will spin to the center position
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, angle_to_compare(0)));

    ESP_LOGI(TAG, "Set generator action on timer and compare event");
    // go high on counter empty
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator,
                                                              MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    // go low on compare threshold
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator,
                                                                MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator, MCPWM_GEN_ACTION_LOW)));

    ESP_LOGI(TAG, "Enable and start timer");
    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));
    }

// Biến để theo dõi trạng thái khóa
static bool door_locked = true;
// Biến để theo dõi thời gian thẻ được lấy đi
static TickType_t card_removed_time = 0;
// Hằng số thời gian tự động đóng khóa sau khi thẻ được lấy đi (5 giây)
#define AUTO_LOCK_DELAY_MS 5000

void servo_task(void *arg)
{
    // Khởi tạo servo
    servo_initial();
    
    // Vòng lặp chính để kiểm tra trạng thái thẻ
    while (1) {
        // Kiểm tra xem có thẻ đang đặt trên đầu đọc không
        if (card_present && card_valid) {
            // Nếu cửa đang khóa, mở khóa ngay lập tức
            if (door_locked) {
                ESP_LOGI(TAG, "Mở khóa - Thẻ hợp lệ");
                ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, angle_to_compare(90)));
                vTaskDelay(500); // Đợi servo xoay
                door_locked = false;
            }
        } else if (!card_present && !door_locked) {
            // Nếu thẻ đã được lấy đi và cửa đang mở
            if (card_removed_time == 0) {
                // Lưu thời điểm thẻ được lấy đi
                card_removed_time = xTaskGetTickCount();
                ESP_LOGI(TAG, "Thẻ đã được lấy đi, sẽ tự động đóng khóa sau 5 giây");
            } else {
                // Kiểm tra xem đã đến lúc đóng khóa chưa
                TickType_t current_time = xTaskGetTickCount();
                if ((current_time - card_removed_time) >= pdMS_TO_TICKS(AUTO_LOCK_DELAY_MS)) {
                    // Đóng khóa
                    ESP_LOGI(TAG, "Tự động đóng khóa sau 5 giây");
                    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, angle_to_compare(0)));
                    vTaskDelay(500); // Đợi servo xoay
                    door_locked = true;
                    card_removed_time = 0; // Reset thời gian
                }
            }
        } else if (card_present && !card_valid) {
            // Nếu thẻ không hợp lệ, đảm bảo cửa đang khóa
            if (!door_locked) {
                ESP_LOGI(TAG, "Đóng khóa - Thẻ không hợp lệ");
                ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, angle_to_compare(0)));
                vTaskDelay(500); // Đợi servo xoay
                door_locked = true;
            }
            card_removed_time = 0; // Reset thời gian
        }
        
        // Đợi một khoảng thời gian ngắn trước khi kiểm tra lại
        vTaskDelay(50); // Kiểm tra mỗi 50 ticks
    }
}