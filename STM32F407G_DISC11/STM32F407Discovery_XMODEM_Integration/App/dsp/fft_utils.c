#include "fft_utils.h"

// List of CMSIS-DSP supported FFT lengths (must be power-of-two)
static const uint16_t supported_fft_lengths[] = {
    16, 32, 64, 128, 256, 512, 1024, 2048, 4096
};

#define NUM_SUPPORTED_LENGTHS (sizeof(supported_fft_lengths)/sizeof(supported_fft_lengths[0]))

// Internal: round down to next power of two
static uint16_t round_down_pow2(uint16_t n) {
    uint16_t p = 1;
    while (p * 2 <= n)
        p *= 2;
    return p;
}

// Public: check if FFT length is supported
bool is_valid_fft_length(uint16_t len) {
    for (uint16_t i = 0; i < NUM_SUPPORTED_LENGTHS; ++i) {
        if (supported_fft_lengths[i] == len)
            return true;
    }
    return false;
}

// Public: get best supported FFT length <= requested
uint16_t get_supported_fft_length(uint16_t requested_len) {
    uint16_t pow2_len = round_down_pow2(requested_len);

    // Search backwards from pow2_len
    for (int i = NUM_SUPPORTED_LENGTHS - 1; i >= 0; --i) {
        if (supported_fft_lengths[i] <= pow2_len)
            return supported_fft_lengths[i];
    }

    // Fallback: minimum allowed
    return supported_fft_lengths[0];
}

/**
 * @brief Apply a Blackman window to a float32 signal array.
 *
 * This function multiplies each sample of the input signal by the corresponding value
 * of the Blackman window, which helps reduce spectral leakage in FFT analysis.
 *
 * The Blackman window is defined as:
 *   w(n) = a0 - a1 * cos(2πn / (N-1)) + a2 * cos(4πn / (N-1))
 *
 * where:
 *   - N is the total number of samples (`length`)
 *   - n is the current sample index (0 to N-1)
 *   - a0 = 0.42, a1 = 0.5, a2 = 0.08
 *
 * @param[in,out] data   Pointer to float32 signal array to apply the window on
 * @param[in]     length Number of elements in the signal array
 */
void apply_blackman_window(float32_t *data, uint32_t length)
{
    // Define Blackman window coefficients
    const float32_t a0 = 0.42f;
    const float32_t a1 = 0.50f;
    const float32_t a2 = 0.08f;

    // Precompute denominator (N-1) to avoid redundant computation
    const float32_t N_minus_1 = (float32_t)(length - 1);

    for (uint32_t i = 0; i < length; i++) {
        // Convert index to float for calculation
        float32_t n = (float32_t)i;

        // Calculate the Blackman window value at index n
        float32_t w = a0
                    - a1 * cosf((2.0f * PI * n) / N_minus_1)
                    + a2 * cosf((4.0f * PI * n) / N_minus_1);

        // Apply the window to the data in-place
        data[i] *= w;
    }
}
