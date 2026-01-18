#include "retarget.h"
#include <stdio.h>

static UART_HandleTypeDef *g_huart = NULL;

void RetargetInit(UART_HandleTypeDef *huart)
{
    g_huart = huart;

    /* optional: make stdout unbuffered so printf appears immediately */
    setvbuf(stdout, NULL, _IONBF, 0);
}

/* GCC / newlib: printf ultimately calls this */
int __io_putchar(int ch)
{
    if (g_huart == NULL)
        return 0;

    HAL_UART_Transmit(g_huart, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
