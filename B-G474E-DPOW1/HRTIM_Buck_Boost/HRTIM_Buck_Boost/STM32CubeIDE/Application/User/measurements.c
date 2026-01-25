/*
 * measurements.c
 */
#include "measurements.h"
#include <stdio.h>
#include "main.h"
#include <stdio.h>

uint16_t adc2_buf[ADC2_BUF_SIZE];

/* --- ADC reference and resolution --- */
#define MEAS_VDDA_mV           3300u      /* ADC reference in mV           */
#define MEAS_ADC_FS_COUNTS     0x0FFFu    /* 12-bit full scale = 4095      */

/* --- Scaling factors for resistor dividers (board MB1428) --- */
/* VIN divider: 27k / 6.8k  → gain ≈ 4.97  → approximated as 497/100      */
#define MEAS_VIN_SCALE_NUM     497u
#define MEAS_VIN_SCALE_DEN     100u

/* VOUT divider: 13.3k / 3.3k → gain ≈ 5.03 → approximated as 503/100     */
#define MEAS_VOUT_SCALE_NUM    503u
#define MEAS_VOUT_SCALE_DEN    100u

static ADC_HandleTypeDef *s_hadc = NULL;

void MEAS_Init(ADC_HandleTypeDef *hadc)
{
    s_hadc = hadc;
}

void MEAS_ReadVoltages(Meas_Voltages_ADC1_t *adc1_meas)
{
    uint32_t tmp;

    if (s_hadc == NULL || adc1_meas == NULL)
        return;

    /* ----- BUCKBOOST_I_IN_SENSE on injected rank 1 ----- */
    adc1_meas->BUCKBOOST_I_IN_SENSE_RAW = HAL_ADCEx_InjectedGetValue(s_hadc, ADC_INJECTED_RANK_1);
    adc1_meas->BUCKBOOST_I_IN_SENSE_mV = (adc1_meas->BUCKBOOST_I_IN_SENSE_RAW * MEAS_VDDA_mV) / MEAS_ADC_FS_COUNTS;
    adc1_meas->BUCKBOOST_I_IN_SENSE_SCALED = (adc1_meas->BUCKBOOST_I_IN_SENSE_mV * MEAS_VIN_SCALE_NUM) / MEAS_VIN_SCALE_DEN;

    adc1_meas->BUCKBOOST_VIN_RAW = HAL_ADCEx_InjectedGetValue(s_hadc, ADC_INJECTED_RANK_2);
    adc1_meas->BUCKBOOST_VIN_mV = (adc1_meas->BUCKBOOST_VIN_RAW * MEAS_VDDA_mV) / MEAS_ADC_FS_COUNTS;
    adc1_meas->BUCKBOOST_VIN_SCALED = (adc1_meas->BUCKBOOST_VIN_mV * MEAS_VIN_SCALE_NUM) / MEAS_VIN_SCALE_DEN;

    adc1_meas->BUCKBOOST_I_IN_AVG_RAW = HAL_ADCEx_InjectedGetValue(s_hadc, ADC_INJECTED_RANK_3);
    adc1_meas->BUCKBOOST_I_IN_AVG_mV = (adc1_meas->BUCKBOOST_I_IN_AVG_RAW * MEAS_VDDA_mV) / MEAS_ADC_FS_COUNTS;
    adc1_meas->BUCKBOOST_I_IN_AVG_SCALED = (adc1_meas->BUCKBOOST_I_IN_AVG_mV * MEAS_VIN_SCALE_NUM) / MEAS_VIN_SCALE_DEN;

    adc1_meas->BUCKBOOST_VOUT_RAW = HAL_ADCEx_InjectedGetValue(s_hadc, ADC_INJECTED_RANK_4);
    adc1_meas->BUCKBOOST_VOUT_mV = (adc1_meas->BUCKBOOST_VOUT_RAW * MEAS_VDDA_mV) / MEAS_ADC_FS_COUNTS;
    adc1_meas->BUCKBOOST_VOUT_SCALED = (adc1_meas->BUCKBOOST_VOUT_mV * MEAS_VOUT_SCALE_NUM) / MEAS_VOUT_SCALE_DEN;

}


void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
  if (hadc->Instance == ADC2) {
    // process first half of adc2_buf[]
	  printf("Half DMA done\r\n");
	  HAL_GPIO_TogglePin(DBG1_PB0_GPIO_Port, DBG1_PB0_Pin);
  }
  HAL_GPIO_TogglePin(DBG1_PB0_GPIO_Port, DBG1_PB0_Pin);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
  if (hadc->Instance == ADC2) {
    // process second half of adc2_buf[]
	  printf("Full DMA done\r\n");
	  HAL_GPIO_TogglePin(DBG1_PB0_GPIO_Port, DBG1_PB0_Pin);
  }
}
