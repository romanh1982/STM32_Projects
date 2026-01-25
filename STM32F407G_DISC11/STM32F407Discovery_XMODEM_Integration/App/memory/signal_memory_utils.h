/*
 * signal_memory_utils.h
 *
 *  Created on: June 3, 2025
 *      Author: WiTrLab36
 *
 *  Description:
 *      Utility functions for signal memory buffer processing.
 *      Provides in-place conversion of ADC-style uint16_t buffers
 *      to signed Q15 fixed-point format, suitable for DSP processing.
 */

#ifndef SIGNAL_MEMORY_UTILS_H_
#define SIGNAL_MEMORY_UTILS_H_

#include <stdint.h>

/**
 * @brief  In-place conversion of ADC uint16_t samples to signed Q15_t format.
 *
 * This function reuses the memory of an existing uint16_t buffer.
 * It first centers the unsigned ADC samples around zero by subtracting
 * the ADC midpoint, and then scales the values to fill the full Q15 range.
 *
 * @param[in,out] pBufU16    Pointer to buffer of uint16_t ADC samples (in-place overwrite as q15_t).
 * @param[in]     blockSize  Number of samples to convert.
 * @param[in]     adcBits    Number of ADC resolution bits (e.g., 12 for 12-bit ADC).
 *
 * @note After conversion:
 *       - Buffer must be interpreted as q15_t*.
 *       - Original ADC values are overwritten.
 *       - Function uses CMSIS-DSP optimized routines (`arm_offset_q15`, `arm_scale_q15`).
 */
void Convert_ADC_U16_to_Q15_InPlace(uint16_t *pBufU16, uint32_t blockSize, uint8_t adcBits);

#endif /* SIGNAL_MEMORY_UTILS_H_ */
