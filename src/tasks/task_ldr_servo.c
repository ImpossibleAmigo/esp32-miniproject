#include "tasks/task_ldr_servo.h"
#include "app_config.h"
#include "drivers/ldr.h"
#include "drivers/servo.h"
#include "utils/math_utils.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <inttypes.h>

static const char *TAG = "task_ldr_servo";

#define LOOP_PERIOD_MS  100

void task_ldr_servo_run(void)
{
    ESP_ERROR_CHECK(ldr_init());
    ESP_ERROR_CHECK(servo_init());

    ESP_LOGI(TAG, "LDR -> Servo controller started (SMA window=%d)", LDR_SMA_WINDOW_SIZE);

    for (;;) {
        int raw = 0;
        int mv = 0;

        if (ldr_read_mv(&raw, &mv) == ESP_OK) {
            int32_t angle = math_map(mv, LDR_LIGHT_MIN_MV, LDR_LIGHT_MAX_MV,
                                     SERVO_MIN_DEG, SERVO_MAX_DEG);
            servo_set_angle(angle);
            ESP_LOGI(TAG, "Raw=%4d | Filtered=%4d mV | Angle=%3" PRId32 " deg",
                     raw, mv, servo_get_angle());
        } else {
            ESP_LOGW(TAG, "Failed to read LDR ADC");
        }

        vTaskDelay(pdMS_TO_TICKS(LOOP_PERIOD_MS));
    }
}