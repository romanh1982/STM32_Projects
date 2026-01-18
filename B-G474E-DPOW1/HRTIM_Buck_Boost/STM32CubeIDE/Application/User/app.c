#include "app.h"
#include "b_g474e_dpow1.h"
#include "measurements.h"
#include <stdio.h>

static Meas_Voltages_t g_voltages;
static uint32_t Vin_mV;
static uint32_t Vout_mV;

#define DE_ENERGIZING_THRESHOLD   ((uint16_t)2500)
#define OVER_VOLTAGE_PROTECTION   ((uint16_t)5000)
#define VDDA                      ((uint16_t)3300)

extern ADC_HandleTypeDef   hadc1;
extern HRTIM_HandleTypeDef hhrtim1;
extern UART_HandleTypeDef  huart3;

static AppMode_t appMode     = APP_MODE_DE_ENERGIZE;
static AppMode_t prevAppMode = (AppMode_t)(-1);

static void APP_UpdateFaultState(void);
static void APP_ReadVoltages(void);
static void APP_HandleStateMachine(void);
static void APP_LogStateIfChanged(void);

void APP_Init(void)
{
    printf("\r\nB-G474E-DPOW1 Motor App starting...\r\n");

    BSP_LED_Init(LED4);
    BSP_LED_Init(LED3);
    BSP_LED_Init(LED5);

    BSP_JOY_Init(JOY1, JOY_MODE_GPIO, JOY_SEL);

    HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
    HAL_ADCEx_InjectedStart(&hadc1);

    /* Startup: de-energize output cap */
    appMode = APP_MODE_DE_ENERGIZE;

    /* HRTIM startup */
    LL_HRTIM_EnableOutput(HRTIM1, LL_HRTIM_OUTPUT_TC1 | LL_HRTIM_OUTPUT_TC2 | LL_HRTIM_OUTPUT_TD1 | LL_HRTIM_OUTPUT_TD2);
    LL_HRTIM_EnableIT_REP(HRTIM1, LL_HRTIM_TIMER_C);
    LL_HRTIM_EnableIT_REP(HRTIM1, LL_HRTIM_TIMER_D);
    LL_HRTIM_TIM_CounterEnable(HRTIM1, LL_HRTIM_TIMER_C);
    LL_HRTIM_TIM_CounterEnable(HRTIM1, LL_HRTIM_TIMER_D);

    HAL_GPIO_WritePin(BUCKBOOST_USBPD_EN_GPIO_Port, BUCKBOOST_USBPD_EN_Pin, GPIO_PIN_SET);

    MEAS_Init(&hadc1);  // pass ADC handle

}

void APP_Task(void)
{
    APP_UpdateFaultState();
    APP_ReadVoltages();
    APP_LogStateIfChanged();
    APP_HandleStateMachine();

    HAL_Delay(50);      // main loop period
}

/* ----------------- static helpers ------------------- */

static void APP_UpdateFaultState(void)
{
    if (LL_HRTIM_IsActiveFlag_FLT2(HRTIM1))
    {
        if (appMode != APP_MODE_FAULT)
            printf("FAULT detected (FLT2 active)\r\n");

        appMode = APP_MODE_FAULT;
    }
}

static void APP_ReadVoltages(void)
{
    MEAS_ReadVoltages(&g_voltages);
    Vin_mV  = g_voltages.vin_mV;
    Vout_mV = g_voltages.vout_mV;

    printf("Vin = %lumV, Vout = %lumV\r\n", Vin_mV, Vout_mV);
}


static void APP_LogStateIfChanged(void)
{
    if (appMode == prevAppMode)
        return;

    prevAppMode = appMode;

    switch (appMode)
    {
        case APP_MODE_BUCK:
            printf("State: BUCK\r\n");
            break;
        case APP_MODE_BOOST:
            printf("State: BOOST\r\n");
            break;
        case APP_MODE_DE_ENERGIZE:
            printf("State: DE_ENERGIZE\r\n");
            break;
        case APP_MODE_FAULT:
            printf("State: FAULT\r\n");
            break;
        default:
            printf("State: UNKNOWN\r\n");
            break;
    }
}

static void APP_HandleStateMachine(void)
{
    switch (appMode)
    {
        case APP_MODE_BUCK:
            if ((BSP_JOY_GetState(JOY1) == JOY_SEL) &&
                (Vin_mV < OVER_VOLTAGE_PROTECTION))
            {
                printf("JOY SEL in BUCK → BOOST\r\n");
                appMode = APP_MODE_BOOST;
                while (BSP_JOY_GetState(JOY1) == JOY_SEL);
            }
            BSP_LED_Toggle(LED4);
            BSP_LED_Off(LED3);
            break;

        case APP_MODE_BOOST:
            if (BSP_JOY_GetState(JOY1) == JOY_SEL)
            {
                printf("JOY SEL in BOOST → DE_ENERGIZE\r\n");
                appMode = APP_MODE_DE_ENERGIZE;
                while (BSP_JOY_GetState(JOY1) == JOY_SEL);
            }
            BSP_LED_Toggle(LED3);
            BSP_LED_Off(LED4);
            break;

        case APP_MODE_DE_ENERGIZE:
            if (Vout_mV < DE_ENERGIZING_THRESHOLD)
            {
                printf("Vout below threshold → BUCK\r\n");
                appMode = APP_MODE_BUCK;
            }
            BSP_LED_Off(LED3);
            BSP_LED_Off(LED5);
            break;

        case APP_MODE_FAULT:
            if (BSP_JOY_GetState(JOY1) == JOY_SEL)
            {
                printf("JOY SEL in FAULT → clear FLT2, DE_ENERGIZE\r\n");
                LL_HRTIM_ClearFlag_FLT2(HRTIM1);
                LL_HRTIM_EnableOutput(HRTIM1, LL_HRTIM_OUTPUT_TC1 | LL_HRTIM_OUTPUT_TC2 | LL_HRTIM_OUTPUT_TD1 | LL_HRTIM_OUTPUT_TD2);

                appMode = APP_MODE_DE_ENERGIZE;
                while (BSP_JOY_GetState(JOY1) == JOY_SEL);
            }
            BSP_LED_Toggle(LED5);
            BSP_LED_Off(LED4);
            BSP_LED_Off(LED3);
            break;

        default:
            LL_HRTIM_DisableOutput(HRTIM1, LL_HRTIM_OUTPUT_TC1 | LL_HRTIM_OUTPUT_TC2 | LL_HRTIM_OUTPUT_TD1 | LL_HRTIM_OUTPUT_TD2);
            break;
    }
}
