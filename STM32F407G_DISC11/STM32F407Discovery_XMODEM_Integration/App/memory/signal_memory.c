/*
 * signal_memory.c
 *
 *  Created on: June 2, 2025
 *      Author: Roman Heinrich
 *
 *  Description:
 *      This source file defines the shared signal processing buffers used across
 *      multiple modules such as signal generation and FFT processing.
 */

#include "signal_memory.h"

/* Frequency array [Hz] for tone generation */
uint32_t freqs_int[MAX_TONES];

/* Amplitude array [mV] for tone generation */
uint16_t amps_int[MAX_TONES];

/**
 * @brief Shared signal buffer instance using a union for flexible data access.
 *
 * This buffer allows shared memory access as either:
 *  - sigBUFFER_UNION.bufF32 → float32_t buffer of length MAX_SIG_LEN
 *  - sigBUFFER_UNION.bufU16 → uint16_t buffer of length MAX_SIG_LEN * 2
 *
 * Primary usage: store raw or processed time-domain signal data (e.g., for signal generation, filtering, FFT).
 *
 * Since this buffer is shared and reused across different signal operations, its contents are *not preserved*.
 * It should only be used when intermediate data does not need to be retained across function calls.
 */
SignalBufferUnion sigBUFFER_UNION;
/**
 * @brief Shared signal buffer instance using a union for flexible data access.
 *
 * This buffer allows shared memory access as either:
 *  - sigBUFFER_UNION.bufF32 → float32_t buffer of length MAX_SIG_LEN
 *  - sigBUFFER_UNION.bufU16 → uint16_t buffer of length MAX_SIG_LEN * 2
 *
 * Primary usage: store raw or processed time-domain signal data (e.g., for signal generation, filtering, FFT).
 *
 * Since this buffer is shared and reused across different signal operations, its contents are *not preserved*.
 * It should only be used when intermediate data does not need to be retained across function calls.
 */
SignalBufferUnion sigBUFFER_UNION;


float32_t sigBUFF2[MAX_SIG_LEN + MAX_NUM_FILTER_TAPS];
