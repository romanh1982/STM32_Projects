/**
 * @file    xmodem_uart_connect.c
 * @brief   UART <-> XMODEM I/O adapter functions for STM32.
 * @author  Roman
 * @date    May 16, 2025
 */

#include "xmodem_uart_connect.h"
#include "uart_app.h"
#include "xmodem_transmitter.h"
#include "xmodem_receiver.h"
/**
 * @brief Check if the UART RX ring buffer is empty.
 *
 * Used by the XMODEM layer to decide whether to wait for new input data.
 *
 * @return true if no new bytes are available; false if data is pending.
 */
static bool is_uart_input_empty(void) {
	bool isUartInputEmpty = uart2_rxRingBuffer.head == uart2_rxRingBuffer.tail;

    return isUartInputEmpty;
}

/**
 * @brief Check if the UART TX path is full.
 *
 * Used by XMODEM before attempting to send a byte.
 * Always returns false here since the UART is assumed to be ready (handled by DMA).
 *
 * @return false, meaning output is always assumed to be available.
 */
static bool is_uart_output_full(void) {
    return false;
}

static bool xmodem_read_data(uint32_t requested_size, uint8_t *buffer, uint32_t *returned_size)
{
    *returned_size = 0;
    for (uint8_t i = 0; i < requested_size; i++) {
        char byte;
        if (!RingBuffer_Read(&uart2_rxRingBuffer, &byte))
            return false;
        buffer[i] = (uint8_t)byte;
        (*returned_size)++;
    }
    return true;
}


/**
 * @brief Write data to UART using blocking transmit.
 *
 * This function uses HAL_UART_Transmit to send `requested_size` bytes from `buffer`
 * through the Debug UART interface.
 *
 * @param[in]  requested_size  Number of bytes to send
 * @param[in]  buffer          Data buffer to send
 * @param[out] write_status    Set to true if write succeeded, false otherwise
 * @return true if transmit was attempted (even if failed), false otherwise
 */
static bool uart_write_data(uint32_t requested_size, uint8_t *buffer, bool *write_status) {
    if (HAL_UART_Transmit(DebugUart, buffer, requested_size, HAL_MAX_DELAY) == HAL_OK) {
        *write_status = true;
        return true;
    }
    *write_status = false;
    return false;
}

/**
 * @brief  Initialize and connect UART-XMODEM I/O callbacks.
 *         Call once before using xmodem_transmit_* or xmodem_receive_*.
 */
void setup_xmodem_callbacks(void)
{
    // Transmit side
    xmodem_transmitter_set_callback_read(xmodem_read_data);
    xmodem_transmitter_set_callback_write(uart_write_data);
    xmodem_transmitter_set_callback_is_inbound_empty(is_uart_input_empty);
    xmodem_transmitter_set_callback_is_outbound_full(is_uart_output_full);

    // Receive side
    xmodem_receive_set_callback_read(xmodem_read_data);
    xmodem_receive_set_callback_write(uart_write_data);
    xmodem_receive_set_callback_is_inbound_empty(is_uart_input_empty);
    xmodem_receive_set_callback_is_outbound_full(is_uart_output_full);
}
