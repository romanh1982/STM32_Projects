#ifndef BOARD_CONFIG_H_
#define BOARD_CONFIG_H_
#include <stdbool.h>
#include "stm32f4xx_hal.h"

extern UART_HandleTypeDef huart2;  // Provided by CubeMX
extern UART_HandleTypeDef huart3;  // Provided by CubeMX

#define DEBUG_UART_HANDLE    (&huart2)  // Or USARTx, etc., configurable per project
#define DEBUG2_UART_HANDLE    (&huart3)  // Or USARTx, etc., configurable per project



void write_BlueLed_PD15(bool state);
void toggle_BlueLed_PD15(void);

void write_RedLed_PD14(bool state);
void toggle_RedLed_PD14(void);

void write_OrangeLed_PD13(bool state);
void toggle_OrangeLed_PD13(void);

void write_GreenLed_PD12(bool state);
void toggle_GreenLed_PD12(void);

void platform_delay_ms(uint32_t ms);
uint32_t platform_get_time_ms(void);


#endif /* BOARD_CONFIG_H_ */
