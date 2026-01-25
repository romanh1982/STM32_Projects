#ifndef FFT_UTILS_H
#define FFT_UTILS_H

#include <stdint.h>
#include <stdbool.h>

#include "arm_math_include.h"

uint16_t get_supported_fft_length(uint16_t requested_len);
bool is_valid_fft_length(uint16_t len);

void apply_blackman_window(float32_t *data, uint32_t length);

#endif // FFT_UTILS_H
