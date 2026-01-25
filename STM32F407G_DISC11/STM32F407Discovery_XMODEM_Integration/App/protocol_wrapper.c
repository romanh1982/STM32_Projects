#include "protocol_wrapper.h"
#include "uart_app.h"
#include <stdio.h>
#include <stdarg.h>

static const char * const cmd_strs[CMD_COUNT] = {
    "READ_FW", "READ_ADC", "WRITE_SERIAL", "UNKNOWN"
};

static const char * const status_strs[RESP_STATUS_COUNT] = {
    "OK", "ERR", "INVALID", "UNKNOWN"
};
#define DEBUG_UART_PRINT_BUFFER_SIZE 128

/**
 * @brief Sends a structured UART response using a response_t struct.
 *
 * This function takes a response descriptor struct (containing command,
 * status, and format string) and a variable argument list for payload
 * formatting. It converts the command and status enums to strings,
 * formats the payload, and sends a complete structured response via UART.
 *
 * Example:
 *   response_t r = { CMD_READ_FW, RESP_OK, "v%s" };
 *   send_formatted_uart_response(&r, SYSTEM_VERSION_STR);
 *
 * @param resp Pointer to the response descriptor struct
 * @param ...  Variable arguments for the payload format
 */
void send_formatted_uart_response(response_cmd_t cmd, response_status_t status, const char *payload_fmt, ...)
{
    const char *cmd_str = cmd < CMD_COUNT ? cmd_strs[cmd] : "INVALID_CMD";
    const char *status_str = status < RESP_STATUS_COUNT ? status_strs[status] : "BAD_STATUS";

    char payload[DEBUG_UART_PRINT_BUFFER_SIZE];
    va_list args;
    va_start(args, payload_fmt);
    vsnprintf(payload, sizeof(payload), payload_fmt, args);
    va_end(args);

    send_uart_response(cmd_str, status_str, "%s", payload);
}
