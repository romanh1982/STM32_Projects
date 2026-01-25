/*
 * signal_memory.h
 *
 *  Created on: June 2, 2025
 *      Author: Roman Heinrich
 *
 *  Description:
 *      This header provides external declarations for shared signal processing buffers.
 *      Only modules that include this header can access the shared memory.
 *
 *      Buffers:
 *          - freqs_int[MAX_TONES]: Frequency array in Hz for tone generation.
 *          - amps_int[MAX_TONES]:  Amplitude array in mV for tone generation.
 *          - signalBuf of type SignalBufferUnion is defined to share the memory between.
 *          	- bufU16[MAX_SIG_LEN]:  Buffer for generated signal samples (ADC codes).
 *          	- bufU16[MAX_SIG_LEN]:  Buffer for generated signal samples (ADC codes).
 */

#ifndef SIGNAL_MEMORY_H_
#define SIGNAL_MEMORY_H_

#include <stdint.h>
#include "arm_math_include.h"

/* Maximum supported tone count and signal length */
#define MAX_TONES    16
#define MAX_SIG_LEN  (1024U * 4U)  /* 4096 samples */
#define MAX_NUM_FILTER_TAPS 256U


/* Union to share memory between uint16_t and float32_t buffers */
typedef union {
    uint16_t bufU16[MAX_SIG_LEN * 2];   /**< Buffer interpreted as uint16_t */
    float32_t bufF32[MAX_SIG_LEN];  /**< Buffer interpreted as float32_t */
} SignalBufferUnion;

/* Extern shared buffer instance */
extern SignalBufferUnion sigBUFFER_UNION;
extern float32_t sigBUFF2[MAX_SIG_LEN + MAX_NUM_FILTER_TAPS];

/* Frequency and amplitude arrays */
extern uint32_t freqs_int[MAX_TONES];
extern uint16_t amps_int[MAX_TONES];

#endif /* SIGNAL_MEMORY_H_ */

