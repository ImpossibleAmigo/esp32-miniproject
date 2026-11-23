#pragma once

#include <driver/ledc.h>
#include <esp_adc/adc_oneshot.h>

/* ============== ВИБІР АКТИВНОГО ЗАВДАННЯ ============== */
#define TASK_LDR_SERVO          1
#define ACTIVE_TASK             TASK_LDR_SERVO

/* ============== LDR (ADC1) ============== */
#define LDR_ADC_UNIT            ADC_UNIT_1
#define LDR_ADC_CHANNEL         ADC_CHANNEL_8   // GPIO 9 на ESP32-S3
#define LDR_ADC_ATTEN           ADC_ATTEN_DB_12
#define LDR_ADC_BITWIDTH        ADC_BITWIDTH_12
#define LDR_SMA_WINDOW_SIZE     8
#define LDR_LIGHT_MIN_MV        50
#define LDR_LIGHT_MAX_MV        3000

/* ============== SERVO (LEDC Timer 0) ============== */
#define SERVO_GPIO              14
#define SERVO_FREQ_HZ           50U
#define SERVO_LEDC_TIMER        LEDC_TIMER_0
#define SERVO_LEDC_CHANNEL      LEDC_CHANNEL_0
#define SERVO_LEDC_RESOLUTION   LEDC_TIMER_12_BIT
#define SERVO_MIN_DEG           10
#define SERVO_MAX_DEG           170
#define SERVO_MIN_US            500
#define SERVO_MAX_US            2500