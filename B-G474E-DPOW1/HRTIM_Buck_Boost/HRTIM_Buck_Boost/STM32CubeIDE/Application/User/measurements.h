/*
 * measurements.h
 */
#ifndef MEASUREMENTS_H
#define MEASUREMENTS_H

#include "stm32g4xx_hal.h"
#include <stdint.h>


#pragma once

#define ADC2_BUF_SIZE 1024
extern uint16_t adc2_buf[ADC2_BUF_SIZE];

void ProcessCurrentSamples_FirstHalf(void);
void ProcessCurrentSamples_SecondHalf(void);


typedef struct
{
    uint32_t vin_mV;
    uint32_t vout_mV;
} Meas_Voltages_t;

/* must be called once with the ADC handle */
void MEAS_Init(ADC_HandleTypeDef *hadc);

/* read Vin / Vout in millivolts using injected channels 1 and 2 */
void MEAS_ReadVoltages(Meas_Voltages_t *v);

#endif /* MEASUREMENTS_H */

