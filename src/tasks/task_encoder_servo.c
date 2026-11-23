#include "tasks/task_encoder_servo.h"
#include "app_config.h"
#include "drivers/buzzer.h"
#include "drivers/encoder.h"
#include "drivers/servo.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <inttypes.h>
#include <stdbool.h>

static const char *TAG = "task_enc_servo";

#define STEP_COARSE_DEG     10
#define STEP_FINE_DEG       (STEP_COARSE_DEG / 2)

void task_encoder_servo_run(void)
{
    ESP_ERROR_CHECK(servo_init());
    ESP_ERROR_CHECK(buzzer_init());
    ESP_ERROR_CHECK(encoder_init());

    QueueHandle_t q = encoder_get_queue();
    bool fine_mode = false;
    int32_t step = STEP_COARSE_DEG;

    ESP_LOGI(TAG, "Ready. Angle=%" PRId32 " deg | Mode=COARSE (%" PRId32 " deg/tick)",
             servo_get_angle(), step);

    encoder_event_t evt;
    for (;;) {
        if (xQueueReceive(q, &evt, portMAX_DELAY) != pdTRUE) continue;

        switch (evt) {
            case ENC_EVT_CW:
            case ENC_EVT_CCW: {
                int32_t delta = (evt == ENC_EVT_CW) ? step : -step;
                esp_err_t res = servo_move_by(delta);
                if (res == ESP_ERR_INVALID_ARG) {
                    ESP_LOGW(TAG, "Limit reached at %" PRId32 " deg!", servo_get_angle());
                    buzzer_play_limit_beep();
                } else {
                    ESP_LOGI(TAG, "Angle=%" PRId32 " deg", servo_get_angle());
                }
                break;
            }

            case ENC_EVT_BTN_SHORT:
                fine_mode = !fine_mode;
                step = fine_mode ? STEP_FINE_DEG : STEP_COARSE_DEG;
                ESP_LOGI(TAG, "Precision changed -> %s (%" PRId32 " deg/tick)",
                         fine_mode ? "FINE" : "COARSE", step);
                break;

            case ENC_EVT_BTN_LONG:
                servo_center();
                ESP_LOGI(TAG, "Servo centered -> %" PRId32 " deg", servo_get_angle());
                buzzer_tone(1200, 80);
                break;
        }
    }
}