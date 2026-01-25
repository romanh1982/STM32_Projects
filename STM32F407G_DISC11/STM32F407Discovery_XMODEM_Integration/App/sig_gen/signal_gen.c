/*
 * signal_gen.c
 *
 *  Refactored signal generator with flexible configuration via SignalGen_Handle_t.
 *
 *  Author: roman.heinrich (refactored)
 *  Date:   May 2025
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>


#include "signal_gen.h"
#include "board_config.h"
#include "signal_memory.h"
#include "uart_app.h"
#include "parse_utils.h"
#include "rng.h"

extern RNG_HandleTypeDef hrng;


#define SIGNAL_CONFIG_OK       (0)
#define SIGNAL_CONFIG_FAIL     (-1)


float32_t generate_noise_mV(float32_t noise_amplitude_mV)
{
    uint32_t rand_val = 0;
    if (HAL_RNG_GenerateRandomNumber(&hrng, &rand_val) != HAL_OK) {
        return 0.0f;  // fallback on RNG error
    }

    // Map 32-bit random value to range [-noise_amp, +noise_amp]
    float32_t rand_norm = ((float32_t)(rand_val & 0xFFFF) / 32768.0f) - 1.0f;
    return rand_norm * noise_amplitude_mV;
}

/**
 * @brief  Generate a composite sine wave signal using Q15-based sine lookup.
 *
 * This function generates a sum of multiple sine waves (multi-tone signal)
 * with specified frequencies and amplitudes, adds a DC offset, clamps the
 * output to a reference voltage range, and rescales the result to unsigned
 * ADC code values (e.g., 12-bit ADC output range 0–4095).
 *
 * Internally, the function uses a fixed-point Q15 representation for phase
 * accumulation and sine computation to achieve efficient signal generation
 * without floating-point math.
 *
 * @param[in] h  Pointer to a fully initialized SignalGen_HandleType:
 *            - samplingRate_u32: Sampling rate in Hz
 *            - dcOffset_u16: DC offset to add to the composite signal (in mV)
 *            - vRef_u16: Reference voltage for scaling to uint16_t (in mV)
 *            - adcMaxValue_u16: Maximum ADC code (e.g., 4095 for 12-bit ADC)
 *            - numTones_u8: Number of sine components
 *            - pToneFreqs_u32: Pointer to array[numTones] of frequencies in Hz
 *            - pToneAmps_u16: Pointer to array[numTones] of amplitudes in mV
 *            - pOutBuffer_u16: Pointer to output buffer (uint16_t), must hold at least numSamples_u16 entries
 *
 * @note   Signal Generation Steps:
 *         1. For each tone, a phase accumulator is initialized and incremented
 *            per sample based on the tone frequency.
 *         2. For each sample:
 *              - Sum all sine waves (amplitude-weighted).
 *              - Add a DC offset (in mV).
 *              - Clamp the result between 0 and vRef_u16 (mV range).
 *              - Rescale the result to fit into [0, adcMaxValue_u16].
 *         3. Store the result as an unsigned 16-bit integer (uint16_t).
 *
 * @note   Q15 Sine Lookup:
 *         - The sine function is evaluated using `arm_sin_q15()` from the CMSIS-DSP library.
 *         - The phase accumulator and increment are scaled to 16-bit fixed-point range (0…65535).
 *         - This avoids the need for floating-point computation and enables fast, efficient signal generation.
 *
 * @note   Intended Use:
 *         - The output signal mimics the output of an ADC: unsigned integer values
 *           corresponding to sampled analog voltages between 0 and vRef_u16 mV.
 *         - If the generated signal is to be analyzed by FFT, post-processing
 *           must re-center the signal around 0 (e.g., subtract DC midpoint)
 *           and possibly convert to a signed type (e.g., q15_t or float32_t).
 */
void SignalGen_GenerateComposite_Q15(SignalGen_HandleType *h)
{
    uint16_t phaseInc[MAX_TONES];
    uint16_t phaseAcc[MAX_TONES] = { 0U };

    /* Precompute per-tone phase increments */
    for (uint8_t k = 0U; k < h->numTones_u8; ++k) {
        uint32_t tmp = ((uint64_t)h->pToneFreqs_u32[k] << 16U) / h->samplingRate_u32;
        phaseInc[k] = (uint16_t)tmp;
    }

    /* Generate each sample */
    for (uint16_t i = 0U; i < h->numSamples_u16; ++i) {
        int32_t sum_mV = (int32_t)h->dcOffset_u16;

        for (uint8_t k = 0U; k < h->numTones_u8; ++k) {
            phaseAcc[k] += phaseInc[k];
            q15_t s = arm_sin_q15((q15_t)(phaseAcc[k] >> 1U));
            sum_mV += ((int32_t)s * h->pToneAmps_u16[k]) >> 15U;
        }

        /* Clamp to [0, vRef] */
        if (sum_mV < 0) {
            sum_mV = 0;
        } else if ((uint32_t)sum_mV > h->vRef_u16) {
            sum_mV = (int32_t)h->vRef_u16;
        }

        /* Scale to ADC code */
        uint32_t code = ((uint32_t)sum_mV * h->adcMaxValue_u16 + h->vRef_u16 / 2U) / h->vRef_u16;
        h->pOutBuffer_u16[i] = (uint16_t)code;
    }
}


/**
 * @brief  Generate a composite sine wave signal consisting of several frequency tones
 *
 * @param[in] sig_handle  Pointer to a fully initialized SignalGen_HandleType:
 *            - samplingRate_u32: Sampling rate in Hz
 *            - dcOffset_u16: DC offset in mV
 *            - vRef_u16: Reference voltage in mV
 *            - adcMaxValue_u16: Maximum ADC code (e.g., 4095 for 12-bit ADC)
 *            - numTones_u8: Number of sine components
 *            - pToneFreqs_u32: Pointer to array[numTones] of frequencies in Hz
 *            - pToneAmps_u16: Pointer to array[numTones] of amplitudes in mV
 *            - pOutBuffer_f32: Pointer to float32_t output buffer (optional)
 *            - pOutBuffer_u16: Pointer to uint16_t output buffer (optional)
 *            - sineMethod: Sine computation method (CMSIS or standard library)
 *            - dataType: Target output data type (float32 or uint16_t ADC codes)
 *
 * @note   For each sample:
 *         - Computes sum of DC offset plus sine components.
 *         - Sine phase angle: 2π·f·t
 *         - If CMSIS sine method is selected, uses arm_sin_f32().
 *         - Otherwise, uses standard sinf().
 *         - Floating point waveform is stored if requested.
 *         - ADC codes are scaled and clamped if requested.
 */
void SignalGen_GenerateComposite(SignalGen_HandleType *sig_handle)
{
    // Calculate the time step between two samples [s] from the sampling rate
    const float32_t timeStep_f32 = 1.0f / (float32_t)sig_handle->samplingRate_u32;

    // Initialize the time variable to start at t = 0
    float32_t t_f32 = 0.0f;

    // Loop through each sample to generate the composite waveform
    for (uint16_t i = 0U; i < sig_handle->numSamples_u16; ++i) {

        // Start with DC offset (in mV)
        float32_t sum_mV = (float32_t)sig_handle->dcOffset_u16;

        // Loop over each sine tone and add its contribution
        for (uint8_t k = 0U; k < sig_handle->numTones_u8; ++k) {

            // Frequency of the current tone [Hz]
            float32_t freq_f32 = (float32_t)sig_handle->pToneFreqs_u32[k];

            // Compute the sine wave phase angle θ = 2π·f·t
            float32_t angle_f32 = 2.0f * PI * freq_f32 * t_f32;

            // Compute the sine wave value using CMSIS or standard math library
            float32_t s_f32 = (sig_handle->sineMethod == SINE_METHOD_CMSIS)
                                ? arm_sin_f32(angle_f32)
                                : sinf(angle_f32);

            // Multiply sine by its amplitude (in mV) and add to sum
            sum_mV += (float32_t)sig_handle->pToneAmps_u16[k] * s_f32;

        }

        // If float32 output is requested, store result in float buffer
        if ((sig_handle->dataType == DATA_TYPE_FLOAT32) && (sig_handle->pOutBuffer_f32 != NULL)) {

            float32_t noise = generate_noise_mV(5.0f);  // adjustable noise amplitude
            sum_mV += noise;							// add noise to the signal
            sig_handle->pOutBuffer_f32[i] = sum_mV;
        }

        // If uint16 (ADC code) output is requested, scale and clip
        if ((sig_handle->dataType == DATA_TYPE_UINT16) && (sig_handle->pOutBuffer_u16 != NULL)) {

            // Normalize mV to range [0.0, 1.0] relative to reference voltage
            float32_t ratio = sum_mV / (float32_t)sig_handle->vRef_u16;

            // Scale normalized value to ADC code range and round
            uint32_t code_u32 = (uint32_t)((ratio * (float32_t)sig_handle->adcMaxValue_u16) + 0.5f);

            // Clamp value to max ADC range
            if (code_u32 > sig_handle->adcMaxValue_u16) {
                code_u32 = (uint32_t)sig_handle->adcMaxValue_u16;
            }

            // Store as uint16 ADC code
            sig_handle->pOutBuffer_u16[i] = (uint16_t)code_u32;
        }

        // Advance time by one sample period
        t_f32 += timeStep_f32;
    }
}
