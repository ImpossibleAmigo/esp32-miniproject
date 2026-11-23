#pragma once

#include <esp_err.h>
#include <stdint.h>

esp_err_t servo_init(void);
esp_err_t servo_set_angle(int32_t angle_deg);
esp_err_t servo_move_by(int32_t delta_deg);
esp_err_t servo_center(void);
int32_t   servo_get_angle(void);