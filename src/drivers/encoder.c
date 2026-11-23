#include "drivers/encoder.h"
#include "app_config.h"

#include <driver/gpio.h>
#include <driver/pulse_cnt.h>
#include <freertos/task.h>
#include <stdbool.h>

#define ENC_POLL_PERIOD_MS  10
#define ENC_QUEUE_LENGTH    32

static pcnt_unit_handle_t s_pcnt_unit = NULL;
static QueueHandle_t      s_evt_queue = NULL;

static void push_event(encoder_event_t evt)
{
    xQueueSend(s_evt_queue, &evt, 0);
}

static void encoder_poll_task(void *arg)
{
    (void)arg;

    int last_count = 0;
    int acc = 0;

    bool btn_stable = false;
    bool btn_raw_prev = false;
    int  debounce_cnt = 0;
    TickType_t press_start = 0;
    bool long_fired = false;

    for (;;) {
        /* 1. Зчитування обертання з апаратного PCNT */
        int count = 0;
        pcnt_unit_get_count(s_pcnt_unit, &count);
        int delta = count - last_count;
        last_count = count;

        if (count > 16000 || count < -16000) {
            pcnt_unit_clear_count(s_pcnt_unit);
            last_count = 0;
        }

        acc += delta;
        while (acc >= ENC_STEPS_PER_DETENT) {
            push_event(ENC_EVT_CW);
            acc -= ENC_STEPS_PER_DETENT;
        }
        while (acc <= -ENC_STEPS_PER_DETENT) {
            push_event(ENC_EVT_CCW);
            acc += ENC_STEPS_PER_DETENT;
        }

        /* 2. Антибрязкіт кнопки та розпізнавання короткого/довгого натискання */
        bool btn_raw = (gpio_get_level(ENC_PIN_SW) == 0); // active-low
        if (btn_raw == btn_raw_prev) {
            if (debounce_cnt < ENC_DEBOUNCE_TICKS) {
                debounce_cnt++;
            } else if (btn_stable != btn_raw) {
                btn_stable = btn_raw;
                if (btn_stable) {
                    press_start = xTaskGetTickCount();
                    long_fired = false;
                } else if (!long_fired) {
                    push_event(ENC_EVT_BTN_SHORT);
                }
            }
        } else {
            debounce_cnt = 0;
            btn_raw_prev = btn_raw;
        }

        if (btn_stable && !long_fired) {
            TickType_t held = xTaskGetTickCount() - press_start;
            if (held >= pdMS_TO_TICKS(ENC_LONG_PRESS_MS)) {
                long_fired = true;
                push_event(ENC_EVT_BTN_LONG);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(ENC_POLL_PERIOD_MS));
    }
}

esp_err_t encoder_init(void)
{
    s_evt_queue = xQueueCreate(ENC_QUEUE_LENGTH, sizeof(encoder_event_t));
    if (!s_evt_queue) return ESP_ERR_NO_MEM;

    /* Налаштування кнопки (внутрішній pull-up обов'язковий для KY-040) */
    const gpio_config_t sw_cfg = {
        .pin_bit_mask = (1ULL << ENC_PIN_SW),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&sw_cfg);
    if (err != ESP_OK) return err;

    /* Налаштування PCNT у квадратурному режимі x4 */
    const pcnt_unit_config_t unit_cfg = {
        .high_limit = 32000,
        .low_limit  = -32000,
    };
    err = pcnt_new_unit(&unit_cfg, &s_pcnt_unit);
    if (err != ESP_OK) return err;

    const pcnt_glitch_filter_config_t filter_cfg = {
        .max_glitch_ns = 1000,
    };
    err = pcnt_unit_set_glitch_filter(s_pcnt_unit, &filter_cfg);
    if (err != ESP_OK) return err;

    pcnt_channel_handle_t chan_a = NULL;
    pcnt_channel_handle_t chan_b = NULL;

    const pcnt_chan_config_t cfg_a = {.edge_gpio_num = ENC_PIN_A, .level_gpio_num = ENC_PIN_B};
    const pcnt_chan_config_t cfg_b = {.edge_gpio_num = ENC_PIN_B, .level_gpio_num = ENC_PIN_A};
    ESP_ERROR_CHECK(pcnt_new_channel(s_pcnt_unit, &cfg_a, &chan_a));
    ESP_ERROR_CHECK(pcnt_new_channel(s_pcnt_unit, &cfg_b, &chan_b));

    /* Страхувальна підтяжка фаз на випадок відсутніх резисторів на клоні KY-040 */
    gpio_set_pull_mode(ENC_PIN_A, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(ENC_PIN_B, GPIO_PULLUP_ONLY);

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(
        chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(
        chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    ESP_ERROR_CHECK(pcnt_unit_enable(s_pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(s_pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(s_pcnt_unit));

    BaseType_t ok = xTaskCreate(encoder_poll_task, "enc_poll", 2048, NULL, 5, NULL);
    return (ok == pdPASS) ? ESP_OK : ESP_FAIL;
}

QueueHandle_t encoder_get_queue(void)
{
    return s_evt_queue;
}