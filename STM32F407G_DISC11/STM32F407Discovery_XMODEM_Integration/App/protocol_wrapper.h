#ifndef PROTOCOL_WRAPPER_H_
#define PROTOCOL_WRAPPER_H_

/* @brief Response status identifiers for UART message formatting. */
typedef enum {
    RESP_OK,
    RESP_ERR,
    RESP_INVALID,
    RESP_UNKNOWN,
    RESP_STATUS_COUNT
} response_status_t;

/* @brief Command identifiers used in UART responses. */
typedef enum {
    CMD_READ_FW,
    CMD_READ_ADC,
    CMD_WRITE_SERIAL,
    CMD_UNKNOWN,
    CMD_COUNT
} response_cmd_t;


/* @brief Structure holding command, status, and format for UART responses. */
typedef struct {
    response_cmd_t cmd;
    response_status_t status;
    const char *payload_fmt;
} response_t;


void send_formatted_uart_response(response_cmd_t cmd, response_status_t status, const char *payload_fmt, ...);


#endif /* PROTOCOL_WRAPPER_H_ */
