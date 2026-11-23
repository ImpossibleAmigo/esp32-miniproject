#pragma once

#include <esp_err.h>
#include <stdint.h>

esp_err_t buzzer_init(void);
void      buzzer_tone(uint32_t freq_hz, uint32_t duration_ms);
void      buzzer_play_limit_beep(void);
void      buzzer_play_error_beep(void);
void      buzzer_play_success_melody(void);
void      buzzer_play_alarm_melody(void);