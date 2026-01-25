/*
 * signal_gen.h
 *
 *  Header for composite signal generator with flexible configuration.
 *
 *  Author: roman.heinrich (refactored)
 *  Date:   May 2025
 */
#include "signal_transfer.h"

#ifndef SIGNAL_GEN_H
#define SIGNAL_GEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "arm_math_include.h"


/** @brief Sine‐generation method */
typedef enum {
    SINE_METHOD_STDLIB = 0,  /**< use standard sinf() */
    SINE_METHOD_CMSIS        /**< use arm_sin_f32() */
} SineMethod_t;

/**
 * @brief Configuration and buffers for the composite-signal generator.
 *
 * The user must populate all fields before calling SignalGen_GenerateComposite().
 * - toneFreqs and toneAmps point to arrays of length numTones.
 * - Depending on dataType, either outFloat32 or outUint16 (or both) must be non-NULL,
 *   and each must point to an array of length numSamples.
 */
typedef struct {
    uint16_t      		numSamples_u16;    	/**< total number of samples to generate */
    uint32_t         	samplingRate_u32;  	/**< sampling rate in Hz */
    uint16_t         	dcOffset_u16;      	/**< DC offset to add to the composite signal */
    uint16_t         	vRef_u16;          	/**< reference voltage for scaling to uint16_t */
    uint16_t      		adcMaxValue_u16;   	/**< maximum ADC code (e.g., 4095 for 12-bit) */
    uint8_t      		numTones_u8;      	/**< number of sine components */
    const uint32_t  	*pToneFreqs_u32;    /**< array[numTones]: frequencies in Hz */
    const uint16_t  	*pToneAmps_u16;     /**< array[numTones]: peak amplitudes */
    SineMethod_t  		sineMethod;    		/**< select sinf() vs. arm_sin_f32() */
    DataType_t    		dataType;      		/**< choose output format (float32_t or uint16_t) */
    float32_t    		*pOutBuffer_f32;    /**< pointer to float32_t output buffer */
    uint16_t     		*pOutBuffer_u16;    /**< pointer to uint16_t output buffer */
} SignalGen_HandleType;


/**
 * @brief Generate a composite signal of multiple sine tones.
 * @param h  Pointer to a fully initialized SignalGen_Handle_t.
 *           See struct comments for required fields.
 */
void SignalGen_GenerateComposite_Q15(SignalGen_HandleType *h);
void SignalGen_GenerateComposite(SignalGen_HandleType *sig_handle);

#ifdef __cplusplus
}
#endif

#endif /* SIGNAL_GEN_H */

