#ifndef RETARGET_H
#define RETARGET_H

#include "stm32g4xx_hal.h"

/* Initialize printf redirection to the given UART */
void RetargetInit(UART_HandleTypeDef *huart);

#endif /* RETARGET_H */
