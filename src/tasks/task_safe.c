#include "tasks/task_safe.h"
#include "app_config.h"
#include "drivers/buzzer.h"
#include "drivers/encoder.h"
#include "drivers/servo.h"

#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define SAFE_PIN_LEN        4
#define SAFE_MAX_ATTEMPTS   3
#define SERVO_LOCKED_DEG    20
#define SERVO_UNLOCKED_DEG  160

static const int TARGET_PIN[SAFE_PIN_LEN] = {3, 7, 2, 5};

static void relay_set_state(bool enabled)
{
    int level = enabled ? RELAY_ACTIVE_LEVEL : !RELAY_ACTIVE_LEVEL;
    gpio_set_level(RELAY_GPIO, level);
}

static void relay_init(void)
{
    const gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << RELAY_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    relay_set_state(false);
}

static void lock_system_forever(void)
{
    printf("\n[SAFE] MAX ATTEMPTS EXCEEDED! SYSTEM LOCKED UNTIL REBOOT.\n");
    fflush(stdout);
    buzzer_play_alarm_melody();

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void print_attempt_prompt(int attempt_num)
{
    printf("\n[SAFE] Attempt %d/%d | PIN: 0", attempt_num, SAFE_MAX_ATTEMPTS);
    fflush(stdout);
}

void task_safe_run(void)
{
    /* Приховуємо INFO-логи драйверів, щоб не ламати рядок із \b */
    esp_log_level_set("*", ESP_LOG_WARN);

    ESP_ERROR_CHECK(servo_init());
    ESP_ERROR_CHECK(buzzer_init());
    ESP_ERROR_CHECK(encoder_init());
    relay_init();

    servo_set_angle(SERVO_LOCKED_DEG);
    QueueHandle_t q = encoder_get_queue();

    int attempts_used = 0;
    int entered[SAFE_PIN_LEN] = {0};
    int digit_idx = 0;
    int current_digit = 0;
    int active_dir = 0; // 0 = ще не задано, +1 = CW, -1 = CCW

    printf("\n=== ELECTRONIC SAFE ===\n");
    printf("Rotate to dial (0-9). Change direction to confirm digit. Press button to RESET.\n");
    print_attempt_prompt(1);

    encoder_event_t evt;
    for (;;) {
        if (xQueueReceive(q, &evt, portMAX_DELAY) != pdTRUE) continue;

        /* Натискання кнопки = скидання набору і витрата 1 спроби */
        if (evt == ENC_EVT_BTN_SHORT || evt == ENC_EVT_BTN_LONG) {
            attempts_used++;
            printf(" [RESET] (%d/%d attempts used)\n", attempts_used, SAFE_MAX_ATTEMPTS);
            fflush(stdout);
            buzzer_play_error_beep();

            if (attempts_used >= SAFE_MAX_ATTEMPTS) {
                lock_system_forever();
            }

            digit_idx = 0;
            current_digit = 0;
            active_dir = 0;
            xQueueReset(q);
            print_attempt_prompt(attempts_used + 1);
            continue;
        }

        int dir = (evt == ENC_EVT_CW) ? 1 : -1;

        if (active_dir == 0 || dir == active_dir) {
            /* Обертання в поточному напрямку інкрементує цифру 0..9 */
            active_dir = dir;
            current_digit = (current_digit + 1) % 10;
            printf("\b%d", current_digit);
            fflush(stdout);
        } else {
            /* Перший тік у протилежний бік підтверджує попередню цифру */
            entered[digit_idx++] = current_digit;

            if (digit_idx < SAFE_PIN_LEN) {
                active_dir = dir;
                current_digit = 0;
                printf("%d", current_digit);
                fflush(stdout);
            } else {
                /* Підтверджено останню цифру — перевіряємо PIN */
                bool match = (memcmp(entered, TARGET_PIN, sizeof(TARGET_PIN)) == 0);
                if (match) {
                    printf("\n[SAFE] ACCESS GRANTED! Unlocking servo and relay...\n");
                    fflush(stdout);
                    servo_set_angle(SERVO_UNLOCKED_DEG);
                    relay_set_state(true);
                    buzzer_play_success_melody();
                    break;
                } else {
                    attempts_used++;
                    printf("\n[SAFE] WRONG PIN! (%d/%d attempts used)\n",
                           attempts_used, SAFE_MAX_ATTEMPTS);
                    fflush(stdout);
                    buzzer_play_error_beep();

                    if (attempts_used >= SAFE_MAX_ATTEMPTS) {
                        lock_system_forever();
                    }

                    digit_idx = 0;
                    current_digit = 0;
                    active_dir = 0;
                    xQueueReset(q);
                    print_attempt_prompt(attempts_used + 1);
                }
            }
        }
    }

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}