#include "app.h"
#include "b_g474e_dpow1.h"
#include "measurements.h"
#include "main.h"
#include <stdio.h>

static Meas_Voltages_t g_voltages;
static uint32_t Vin_mV;
static uint32_t Vout_mV;

#define DE_ENERGIZING_THRESHOLD   ((uint16_t)2500)
#define OVER_VOLTAGE_PROTECTION   ((uint16_t)5000)
#define VDDA                      ((uint16_t)3300)


extern ADC_HandleTypeDef   hadc1;
extern ADC_HandleTypeDef   hadc2;
extern HRTIM_HandleTypeDef hhrtim1;
extern UART_HandleTypeDef  huart3;
extern uint16_t adc2_buf[ADC2_BUF_SIZE];


AppMode_t appMode = APP_MODE_DE_ENERGIZE;
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

    BSP_JOY_Init(JOY1, JOY_MODE_GPIO, JOY_ALL);
    //HAL_StatusTypeDef result = HAL_ADC_Start_DMA(&hadc2, (uint32_t*)adc2_buf, ADC2_BUF_SIZE);
    //printf("HAL_ADC_Start_DMA returns : %02X \r\n", result);

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

    HAL_Delay(1000);      // main loop period
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
    HAL_StatusTypeDef status1, status2, status3 = HAL_OK;

    printf("Vin = %lumV, Vout = %lumV\r\n", Vin_mV, Vout_mV);

    status1 = HAL_ADC_Start(&hadc2);  // Start single conversion
    printf("HAL_ADC_Start = %02X\r\n", status1);
    status2 = HAL_ADC_PollForConversion(&hadc2, 100);
    printf("HAL_ADC_PollForConversion = %02X\r\n", status2);
    if ((status1 == HAL_OK) && (status2 == HAL_OK))
    {
        uint32_t result = HAL_ADC_GetValue(&hadc2);
        printf("ADC2 Result = %lu\r\n", result);
    }
    if ((ADC2->CR & ADC_CR_ADEN) == 0)
    {
        printf("ADC2 NOT ENABLED!\r\n");
        printf("ISR = 0x%08lx\r\n", ADC2->ISR);
        printf("CR = 0x%08lx\r\n",  ADC2->CR);
    }
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
    int32_t joyState = BSP_JOY_GetState(JOY1);

    // Print decoded joystick state
    switch (joyState) {
        case JOY_SEL:   printf("JOY: SELECT\r\n"); break;
        case JOY_UP:    printf("JOY: UP\r\n"); break;
        case JOY_DOWN:  printf("JOY: DOWN\r\n"); break;
        case JOY_LEFT:  printf("JOY: LEFT\r\n"); break;
        case JOY_RIGHT: printf("JOY: RIGHT\r\n"); break;
        case JOY_NONE:  printf("JOY: NONE\r\n"); break;
        default:        printf("JOY: Unknown (0x%02lX)\r\n", joyState); break;
    }

    // Reassign appMode based on direction
    switch (joyState) {
        case JOY_LEFT:
            appMode = APP_MODE_BUCK;
            printf("JOY LEFT → BUCK\r\n");
            break;
        case JOY_RIGHT:
            appMode = APP_MODE_BOOST;
            printf("JOY RIGHT → BOOST\r\n");
            break;
        case JOY_DOWN:
            appMode = APP_MODE_DE_ENERGIZE;
            printf("JOY DOWN → DE_ENERGIZE\r\n");
            break;
        case JOY_UP:
            appMode = APP_MODE_FAULT;
            printf("JOY UP → FAULT\r\n");
            break;
        default:
            break;  // do nothing on SEL or NONE
    }

    // Optional: debounce SELECT if used elsewhere
    if (joyState == JOY_SEL) {
        while (BSP_JOY_GetState(JOY1) == JOY_SEL) {
            HAL_Delay(20);
        }
    }

    printf("APP_MODE: %d\r\n", appMode);

    // Act based on current appMode (LEDs, protection, etc.)
    switch (appMode)
    {
        case APP_MODE_BUCK:
            BSP_LED_Toggle(LED4);
            BSP_LED_Off(LED3);
            break;

        case APP_MODE_BOOST:
            BSP_LED_Toggle(LED3);
            BSP_LED_Off(LED4);
            break;

        case APP_MODE_DE_ENERGIZE:
            if (Vout_mV < DE_ENERGIZING_THRESHOLD) {
                printf("Vout below threshold → BUCK\r\n");
                appMode = APP_MODE_BUCK;
            }
            BSP_LED_Off(LED3);
            BSP_LED_Off(LED5);
            break;

        case APP_MODE_FAULT:
            LL_HRTIM_ClearFlag_FLT2(HRTIM1);
            LL_HRTIM_EnableOutput(HRTIM1,
                LL_HRTIM_OUTPUT_TC1 | LL_HRTIM_OUTPUT_TC2 |
                LL_HRTIM_OUTPUT_TD1 | LL_HRTIM_OUTPUT_TD2);
            BSP_LED_Toggle(LED5);
            BSP_LED_Off(LED4);
            BSP_LED_Off(LED3);
            break;

        default:
            LL_HRTIM_DisableOutput(HRTIM1,
                LL_HRTIM_OUTPUT_TC1 | LL_HRTIM_OUTPUT_TC2 |
                LL_HRTIM_OUTPUT_TD1 | LL_HRTIM_OUTPUT_TD2);
            break;
    }
}


