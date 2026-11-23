#pragma once

#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

typedef enum {
    ENC_EVT_CW = 0,
    ENC_EVT_CCW,
    ENC_EVT_BTN_SHORT,
    ENC_EVT_BTN_LONG,
} encoder_event_t;

esp_err_t     encoder_init(void);
QueueHandle_t encoder_get_queue(void);