#include "drivers/buzzer.h"
#include "app_config.h"

#include <driver/ledc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stddef.h>

#define BUZZER_DUTY_50  ((1UL << BUZZER_LEDC_RESOLUTION) / 2UL)

esp_err_t buzzer_init(void)
{
    const ledc_timer_config_t timer_cfg = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num       = BUZZER_LEDC_TIMER,
        .duty_resolution = BUZZER_LEDC_RESOLUTION,
        .freq_hz         = 1000,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    esp_err_t err = ledc_timer_config(&timer_cfg);
    if (err != ESP_OK) return err;

    const ledc_channel_config_t chan_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = BUZZER_LEDC_CHANNEL,
        .timer_sel  = BUZZER_LEDC_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = BUZZER_GPIO,
        .duty       = 0,
        .hpoint     = 0,
    };
    return ledc_channel_config(&chan_cfg);
}

void buzzer_tone(uint32_t freq_hz, uint32_t duration_ms)
{
    if (freq_hz > 0) {
        ledc_set_freq(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_TIMER, freq_hz);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL, BUZZER_DUTY_50);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL);
    }
    vTaskDelay(pdMS_TO_TICKS(duration_ms));

    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL);
}

void buzzer_play_limit_beep(void)
{
    buzzer_tone(2200, 60);
}

void buzzer_play_error_beep(void)
{
    buzzer_tone(350, 150);
    vTaskDelay(pdMS_TO_TICKS(60));
    buzzer_tone(280, 220);
}

void buzzer_play_success_melody(void)
{
    const uint32_t notes[] = {523, 659, 784, 1047}; // C5-E5-G5-C6
    for (size_t i = 0; i < sizeof(notes) / sizeof(notes[0]); i++) {
        buzzer_tone(notes[i], 110);
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

void buzzer_play_alarm_melody(void)
{
    for (int i = 0; i < 6; i++) {
        buzzer_tone(1800, 180);
        buzzer_tone(1200, 180);
    }
}