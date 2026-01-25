#include "board_config.h"
#include "stm32f4xx_hal_gpio.h"


void write_BlueLed_PD15(bool state)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void toggle_BlueLed_PD15(void)
{
	HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_15);
}

void write_RedLed_PD14(bool state)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void toggle_RedLed_PD14(void)
{
	HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14);
}

void write_OrangeLed_PD13(bool state)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void toggle_OrangeLed_PD13(void)
{
	HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_13);
}

void write_GreenLed_PD12(bool state)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void toggle_GreenLed_PD12(void)
{
	HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12);
}

void platform_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}


uint32_t platform_get_time_ms(void)
{
  return uwTick;
}
