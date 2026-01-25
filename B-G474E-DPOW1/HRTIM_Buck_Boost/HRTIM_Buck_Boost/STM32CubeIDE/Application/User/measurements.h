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
    uint32_t BUCKBOOST_I_IN_SENSE_RAW;
    uint32_t BUCKBOOST_I_IN_SENSE_SCALED;
    uint32_t BUCKBOOST_I_IN_SENSE_mV;

    uint32_t BUCKBOOST_VIN_RAW;
    uint32_t BUCKBOOST_VIN_mV;
    uint32_t BUCKBOOST_VIN_SCALED;

    uint32_t BUCKBOOST_I_IN_AVG_RAW;
    uint32_t BUCKBOOST_I_IN_AVG_mV;
    uint32_t BUCKBOOST_I_IN_AVG_SCALED;

    uint32_t BUCKBOOST_VOUT_RAW;
    uint32_t BUCKBOOST_VOUT_mV;
    uint32_t BUCKBOOST_VOUT_SCALED;
} Meas_Voltages_ADC1_t;

/* must be called once with the ADC handle */
void MEAS_Init(ADC_HandleTypeDef *hadc);

/* read Vin / Vout in millivolts using injected channels 1 and 2 */
void MEAS_ReadVoltages(Meas_Voltages_ADC1_t *v);

#endif /* MEASUREMENTS_H */

