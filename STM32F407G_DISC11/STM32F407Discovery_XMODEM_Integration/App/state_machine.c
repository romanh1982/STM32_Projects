/* state_machine.c - Clean Skeleton Version */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "fft_handle.h"
#include "sig_xmodem_handle.h"
#include "sig_handle.h"
#include "version_handle.h"


#include "uart_app.h"
#include "state_machine.h"

#include "protocol_wrapper.h"
#include "board_config.h"
#include "test_uart_app.h"
#include "xmodem_uart_connect.h"
#include "xmodem_transmitter.h"

#include "signal_gen.h"
#include "json_utils.h"
#define JSMN_HEADER
#include "jsmn.h"  // declares only


// Command Handler Definition
typedef struct {
    const char *command;
    void (*handler)(const char *args);
} command_entry_t;

/* Private function prototypes -----------------------------------------------*/
void execute_command(void);

static const command_entry_t command_table[] = {
    { "READ_FW", handle_read_fw },
	{ "READ_SER", handle_read_ser },
	{ "READ_HW", handle_read_hw },
	{ "XMT_TEST", handle_xmodem_test },
	{"READ_GEN_SIG_FLEX_XMODEM", handle_read_GenSignal_Flex_Xmodem},
	{"READ_FFT", handle_read_fft},
	{"READ_SCALED_SIG", handle_read_scaled_signal},
	{"READ_SIG_FFT", handle_read_sig_fft},
};

#define NUM_COMMANDS (sizeof(command_table) / sizeof(command_table[0]))

volatile uint8_t cmdTransferFinished = 0;

void state_machine(void)
{
	// Start first DMA reception (per‐byte). After the first byte of data is received it will call HAL_UART_RxCpltCallback located uart_app.c, then the callback will be restarted there.
	HAL_UART_Receive_DMA(DEBUG_UART_HANDLE, (uint8_t*)uart2_rxBuf, DMA_BUFFER_SIZE);
	HAL_UART_Receive_DMA(DEBUG2_UART_HANDLE, (uint8_t*)uart3_rxBuf, DMA_BUFFER_SIZE);
	setup_xmodem_callbacks();

	printToDebugUartBlocking("[DBG] Enter command:\r\n");
	printToDebug2UartBlocking("[DBG] Enter command:\r\n");
	stateMachineStatesEnum state = Command_State;

    while (1)
    {
        switch(state)
        {
            case Test_State:
            	test_print_ArrayToUART_Out();
                break;
            case Command_State:
            	execute_command();
            	break;
            default:
                break;
        }
    }
}

void execute_command(void)
{
    if (commandQueue.count > 0)
    {
        char *command = commandQueue.buffer[commandQueue.head];
        // Trim newline
        char *newline = strpbrk(command, "\r\n");
        if (newline) *newline = '\0';

        printToDebugUartBlocking("[DBG] Received: %s \r\n", command);

        bool matched = false;

        // Check for new JSON-style command
        if (command[0] == '{')
        {
            jsmn_parser parser;
            jsmntok_t tokens[MAX_JSON_TOKENS];
            jsmn_init(&parser);

            int tok_count = jsmn_parse(&parser, command, strlen(command), tokens, MAX_JSON_TOKENS);
            if (tok_count > 0 && tokens[0].type == JSMN_OBJECT)
            {
                for (uint32_t i = 0; i < tok_count; i++)
                {
                    if (tokens[i].type == JSMN_STRING && json_token_streq(command, &tokens[i], "cmd"))
                    {
                        jsmntok_t *cmd_val = &tokens[i + 1];
                        for (uint32_t j = 0; j < NUM_COMMANDS; j++)
                        {
                            if (json_token_streq(command, cmd_val, command_table[j].command))
                            {
                                command_table[j].handler(command);  // Pass full JSON
                                matched = true;
                                break;
                            }
                        }
                        break;
                    }
                }
            }
            else{
            	printToDebugUartBlocking("[DBG] [Error] JSON parsing failed. Token error: %d\r\n", tok_count);
            }
        }

        if (!matched)
        {
            printToDebugUartBlocking("[DBG] Command <%s> not recognized.\r\n", command);
            const char *command_read_ser = "{\"cmd\": \"READ_SER\"}\\n";
            printToDebugUartBlocking("[\r\nDBG]Command must be in JSON-Format like <%s>\r\n", command_read_ser);

            const char *command_help = "{\"cmd\": \"HELP\"}\\n";
            printToDebugUartBlocking("[DBG] Enter <%s> to display a list of commands.\r\n",command_help);

        }

        // Advance queue
        commandQueue.head = (commandQueue.head + 1) % COMMAND_BUFFER_SIZE;
        commandQueue.count--;
        printToDebugUartBlocking("\r\n[DBG]:Enter command:\r\n");
    }
}


