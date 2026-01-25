#ifndef XMODEM_UART_CONNECT_H
#define XMODEM_UART_CONNECT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Initialize and connect UART-XMODEM I/O callbacks.
 *         Call once before using xmodem_transmit_* or xmodem_receive_*.
 */
void setup_xmodem_callbacks(void);

#ifdef __cplusplus
}
#endif

#endif
