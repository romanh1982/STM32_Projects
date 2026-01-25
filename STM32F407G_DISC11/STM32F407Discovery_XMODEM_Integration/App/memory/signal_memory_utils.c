/*
 * signal_memory_utils.c
 *
 *  Created on: June 3, 2025
 *      Author: WiTrLab36
 *
 *  Description:
 *      Implementation of signal memory buffer utilities.
 *      Provides efficient, in-place unsigned to signed Q15 conversion.
 */
#define __FPU_PRESENT 1
#include "arm_math.h"
#include "signal_memory_utils.h"

void Convert_ADC_U16_to_Q15_InPlace(uint16_t *pBufU16, uint32_t blockSize, uint8_t adcBits)
{
    int16_t adcMidpoint = (int16_t)(1U << (adcBits - 1U));           /* e.g., 2048 for 12-bit ADC */
    q15_t *pBufQ15 = (q15_t *)pBufU16;

    /* Step 1: Subtract ADC midpoint to center signal around zero */
    arm_offset_q15(pBufQ15, (q15_t)(-adcMidpoint), pBufQ15, blockSize);

    /* Step 2: Scale */
    q15_t scaleFract;

    if (adcBits == 12U) {
        scaleFract = 16 * 1024;  /* Approximate, or simply 32767 / 2047 */
    } else {
        scaleFract = (q15_t)(32767 >> (15U - adcBits));
    }

    /* Actually, best: */
    scaleFract = (q15_t)(32767 / ((1U << (adcBits - 1U)) - 1U));  // 32767 / 2047 for 12-bit ADC

    arm_scale_q15(pBufQ15, scaleFract, 0, pBufQ15, blockSize);
}
