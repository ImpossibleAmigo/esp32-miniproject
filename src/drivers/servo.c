#include "drivers/servo.h"
#include "app_config.h"
#include "utils/math_utils.h"

#include <driver/ledc.h>
#include <esp_log.h>
#include <inttypes.h>
#include <stdbool.h>

static const char *TAG = "servo";

#define SERVO_PERIOD_US     (1000000UL / SERVO_FREQ_HZ)
#define SERVO_MAX_DUTY      ((1UL << SERVO_LEDC_RESOLUTION) - 1UL)

static int32_t s_current_angle = (SERVO_MIN_DEG + SERVO_MAX_DEG) / 2;

esp_err_t servo_init(void)
{
    const ledc_timer_config_t timer_cfg = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num       = SERVO_LEDC_TIMER,
        .duty_resolution = SERVO_LEDC_RESOLUTION,
        .freq_hz         = SERVO_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    esp_err_t err = ledc_timer_config(&timer_cfg);
    if (err != ESP_OK) return err;

    const ledc_channel_config_t chan_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = SERVO_LEDC_CHANNEL,
        .timer_sel  = SERVO_LEDC_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = SERVO_GPIO,
        .duty       = 0,
        .hpoint     = 0,
    };
    err = ledc_channel_config(&chan_cfg);
    if (err != ESP_OK) return err;

    return servo_center();
}

esp_err_t servo_set_angle(int32_t angle_deg)
{
    bool hit_limit = (angle_deg < SERVO_MIN_DEG) || (angle_deg > SERVO_MAX_DEG);
    int32_t clamped = math_clamp(angle_deg, SERVO_MIN_DEG, SERVO_MAX_DEG);

    int32_t pulse_us = math_map(clamped, SERVO_MIN_DEG, SERVO_MAX_DEG,
                                SERVO_MIN_US, SERVO_MAX_US);
    uint32_t duty = (uint32_t)(((uint64_t)pulse_us * SERVO_MAX_DUTY) / SERVO_PERIOD_US);

    esp_err_t err = ledc_set_duty(LEDC_LOW_SPEED_MODE, SERVO_LEDC_CHANNEL, duty);
    if (err != ESP_OK) return err;

    err = ledc_update_duty(LEDC_LOW_SPEED_MODE, SERVO_LEDC_CHANNEL);
    if (err != ESP_OK) return err;

    s_current_angle = clamped;
    ESP_LOGD(TAG, "angle=%" PRId32 " deg, pulse=%" PRId32 " us, duty=%" PRIu32,
             clamped, pulse_us, duty);

    return hit_limit ? ESP_ERR_INVALID_ARG : ESP_OK;
}

esp_err_t servo_move_by(int32_t delta_deg)
{
    return servo_set_angle(s_current_angle + delta_deg);
}

esp_err_t servo_center(void)
{
    return servo_set_angle((SERVO_MIN_DEG + SERVO_MAX_DEG) / 2);
}

int32_t servo_get_angle(void)
{
    return s_current_angle;
}