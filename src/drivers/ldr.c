#include "drivers/ldr.h"
#include "app_config.h"

#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_log.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const char *TAG = "ldr";

static adc_oneshot_unit_handle_t s_adc_handle = NULL;
static adc_cali_handle_t         s_cali_handle = NULL;
static bool                      s_calibrated = false;

static int      s_sma_buf[LDR_SMA_WINDOW_SIZE] = {0};
static size_t   s_sma_idx = 0;
static size_t   s_sma_count = 0;
static int64_t  s_sma_sum = 0;

static int sma_update(int sample)
{
    if (s_sma_count == LDR_SMA_WINDOW_SIZE) {
        s_sma_sum -= s_sma_buf[s_sma_idx];
    } else {
        s_sma_count++;
    }

    s_sma_buf[s_sma_idx] = sample;
    s_sma_sum += sample;
    s_sma_idx = (s_sma_idx + 1U) % LDR_SMA_WINDOW_SIZE;

    return (int)(s_sma_sum / (int64_t)s_sma_count);
}

esp_err_t ldr_init(void)
{
    const adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = LDR_ADC_UNIT,
    };
    esp_err_t err = adc_oneshot_new_unit(&unit_cfg, &s_adc_handle);
    if (err != ESP_OK) return err;

    const adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = LDR_ADC_ATTEN,
        .bitwidth = LDR_ADC_BITWIDTH,
    };
    err = adc_oneshot_config_channel(s_adc_handle, LDR_ADC_CHANNEL, &chan_cfg);
    if (err != ESP_OK) return err;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    const adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id  = LDR_ADC_UNIT,
        .chan     = LDR_ADC_CHANNEL,
        .atten    = LDR_ADC_ATTEN,
        .bitwidth = LDR_ADC_BITWIDTH,
    };
    if (adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_cali_handle) == ESP_OK) {
        s_calibrated = true;
    } else {
        ESP_LOGW(TAG, "ADC calibration unavailable, using linear estimate");
    }
#endif

    return ESP_OK;
}

esp_err_t ldr_read_mv(int *out_raw, int *out_mv_filtered)
{
    int raw = 0;
    esp_err_t err = adc_oneshot_read(s_adc_handle, LDR_ADC_CHANNEL, &raw);
    if (err != ESP_OK) return err;

    int mv = 0;
    if (s_calibrated) {
        err = adc_cali_raw_to_voltage(s_cali_handle, raw, &mv);
        if (err != ESP_OK) return err;
    } else {
        mv = (raw * 3300) / 4095;
    }

    if (out_raw) *out_raw = raw;
    if (out_mv_filtered) *out_mv_filtered = sma_update(mv);
    return ESP_OK;
}