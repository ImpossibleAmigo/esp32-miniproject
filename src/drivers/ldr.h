#pragma once

#include <esp_err.h>

esp_err_t ldr_init(void);
esp_err_t ldr_read_mv(int *out_raw, int *out_mv_filtered);