#pragma once

#include <stdint.h>

static inline int32_t math_clamp(int32_t value, int32_t min_val, int32_t max_val)
{
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

static inline int32_t math_map(int32_t value, int32_t in_min, int32_t in_max,
                               int32_t out_min, int32_t out_max)
{
    if (in_min == in_max) return out_min;

    int32_t clamped = (in_min < in_max)
                          ? math_clamp(value, in_min, in_max)
                          : math_clamp(value, in_max, in_min);

    int64_t num = (int64_t)(clamped - in_min) * (int64_t)(out_max - out_min);
    int64_t den = (int64_t)(in_max - in_min);
    return (int32_t)(out_min + num / den);
}