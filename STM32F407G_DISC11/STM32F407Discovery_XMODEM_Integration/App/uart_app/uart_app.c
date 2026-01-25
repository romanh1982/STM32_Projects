/* uart_app.c  */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "uart_app.h"
#include "board_config.h"

/* Private defines -----------------------------------------------------------*/
#define UART_TX_BUFFER_SIZE 			512	// Max number of queued messages
#define UART_TX_MESSAGE_SIZE 			16	// Max length of a single message


/* Private typedef -----------------------------------------------------------*/
typedef struct {
    char buffer[UART_TX_BUFFER_SIZE][UART_TX_MESSAGE_SIZE];  	// Buffer to hold messages
    uint8_t length[UART_TX_BUFFER_SIZE];       					// store each message's length
    uint8_t head;                                      			// Points to the next message to be transmitted
    uint8_t tail;                                      			// Points to the next empty slot
    uint8_t count;                                     			// Number of queued messages
} UART_MessageQueue;


/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef *DebugUart 	= DEBUG_UART_HANDLE;
UART_HandleTypeDef *Debug2Uart 	= DEBUG2_UART_HANDLE;


/* This is a FIFO buffers that store all incoming bytes in order.
   It decouples the DMA reception from the higher-level parsing logic.
 * When a full line is ready (e.g. ends with \n), it's extracted from here.
*/
UART_RingBuffer uart2_rxRingBuffer = {{0}, 0, 0};
UART_RingBuffer uart3_rxRingBuffer = {{0}, 0, 0};


/* Purpose: This is the 1-byte buffer filled by the DMA for each received byte.
   After every byte, the interrupt is fired, and the byte is moved into the software ring buffer.
 */
volatile char uart2_rxBuf[DMA_BUFFER_SIZE];
volatile char uart3_rxBuf[DMA_BUFFER_SIZE];

/* Purpose: This stores fully parsed commands (one command per line, up to 10 commands in parallel).
 * Each command can be up to 511 characters (plus \0).
 */
CommandBuffer commandQueue = {
		.head = 0,
		.tail = 0,
		.count = 0 };

/* Purpose: This is used for queued debug output when sending messages with DMA.
 * It stores formatted strings for transmission.
 */
volatile UART_MessageQueue uartQueue 	= { .head = 0, .tail = 0, .count = 0 };

volatile uint8_t 		txComplete = 1;  		// 1 means UART is free to send
static volatile uint32_t overflowCounter = 0;	// Counts how many messages were dropped because the TX queue was full.

/* Private function prototypes -----------------------------------------------*/

void RingBuffer_Write(UART_RingBuffer *ringBuffer, char newByte);

/* Private function prototypes -----------------------------------------------*/
/**
 * @brief  Enqueue a byte into a software ring buffer.
 *
 * Attempts to write the given byte into the ring buffer at the current head
 * position. If the buffer is full (i.e. head would catch up to tail), the new
 * byte is silently dropped.
 *
 * @param  ringBuffer  Pointer to the UART_RingBuffer instance to write into.
 * @param  newByte     The byte value to enqueue.
 *
 * @note   The buffer size is SOFTWARE_RING_BUFFER_SIZE. Head and tail wrap
 *         around modulo this size.
 */
void RingBuffer_Write(UART_RingBuffer *ringBuffer, char newByte)
{
    uint16_t next = (ringBuffer->head + 1) % SOFTWARE_RING_BUFFER_SIZE;
    if (next != ringBuffer->tail)
    {
        ringBuffer->buffer[ringBuffer->head] = newByte;
        ringBuffer->head = next;
    }
}

/**
 * @brief  Dequeue a byte from a software ring buffer.
 *
 * Reads one byte from the ring buffer at the current tail position and advances
 * the tail. If the buffer is empty (head equals tail), no data is read.
 *
 * @param  ringBuffer  Pointer to the UART_RingBuffer instance to read from.
 * @param  data        Pointer to the variable where the dequeued byte will be stored.
 * @return int         Returns 1 if a byte was successfully read, or 0 if the buffer was empty.
 *
 * @note   The buffer size is SOFTWARE_RING_BUFFER_SIZE. Head and tail wrap
 *         around modulo this size.
 */
int RingBuffer_Read(UART_RingBuffer *ringBuffer, char *data)
{
    if (ringBuffer->head == ringBuffer->tail)
        return 0;

    *data = ringBuffer->buffer[ringBuffer->tail];
    ringBuffer->tail = (ringBuffer->tail + 1) % SOFTWARE_RING_BUFFER_SIZE;
    return 1;
}

/**
 * @brief  Redirects printf() output to UART.
 *
 * This function is a low-level implementation required by
 * the standard I/O library. It is called by printf(), puts(),
 * and other standard output functions to send characters.
 *
 * In this implementation, each character is transmitted via
 * HAL_UART_Transmit() using the UART handle `DebugUart`.
 *
 * @param  ch Character to transmit
 * @return The transmitted character
 */
int __io_putchar(int ch)
{
    HAL_UART_Transmit(Debug2Uart, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/* Functions -----------------------------------------------------------------*/

/**
 * @brief  Sends a formatted string over debug UART in blocking mode.
 *
 * This function is a convenient wrapper similar to printf(). It formats
 * the input using vsnprintf() and transmits the full resulting message
 * using HAL_UART_Transmit() in blocking mode.
 *
 * Unlike printf(), which sends one character at a time via __io_putchar(),
 * this sends the entire message in one UART transaction.
 *
 * @param format printf-style format string
 * @param ...    Variable arguments matching the format string
 */
void printToDebugUartBlocking(char *format, ...)
{
	char buffer[UART_TX_BUFFER_SIZE];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    HAL_UART_Transmit(DebugUart, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
}

void printToDebug2UartBlocking(char *format, ...)
{
	char buffer[UART_TX_BUFFER_SIZE];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    HAL_UART_Transmit(Debug2Uart, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
}

/**
 * @brief  Sends a structured UART response message in blocking mode.
 *
 * This function formats a response message with a standard prefix and
 * sends it over the debug UART. The message format is:
 *   <RESP:command|status|payload>
 *
 * It is useful for communicating structured responses to a host (e.g. GUI, CLI),
 * especially when parsing UART output.
 *
 * Internally, it uses vsnprintf() to format the payload section, and then wraps
 * that payload into the full response format using printToDebugUartBlocking().
 *
 * @param cmd         The command name being responded to (e.g. "READ_ADC")
 * @param status      Status string (e.g. "OK", "ERR", "INVALID")
 * @param payload_fmt printf-style format string for the response payload
 * @param ...         Variable arguments matching the format string
 */
void send_uart_response(const char *cmd, const char *status, const char *payload_fmt, ...)
{
    char payload[UART_TX_BUFFER_SIZE];
    va_list args;
    va_start(args, payload_fmt);
    vsnprintf(payload, sizeof(payload), payload_fmt, args);
    va_end(args);

    printToDebugUartBlocking("<RESP:%s|%s|%s>\r\n", cmd, status, payload);
}


/**
 * @brief  process_full_command_debug_uart - Queues full JSON or ASCII commands from the debug UART.
 *
 * Reads bytes from the `uart2_rxRingBuffer` into a temporary line buffer until a newline
 * character (`'\n'`) is received or the buffer is full. The full line is then null-terminated
 * and enqueued into the global `commandQueue` as a single command entry.
 *
 * This function supports both legacy ASCII commands (e.g., "READ_FW") and modern JSON-based
 * commands (e.g., {"cmd":"READ_GEN_SIG_FLEX",...}) by treating the entire line as one unit.
 *
 * @note The function no longer tokenizes input using `strtok` — JSON strings are preserved as-is.
 * @note Lines longer than `COMMAND_LENGTH-1` will be truncated before enqueuing.
 * @note Lines without a terminating `\n` will remain unprocessed until a newline is received.
 * @note `commandQueue` must be large enough to hold full commands; if full, new lines are dropped silently.
 */

void process_full_command_debug_uart(void)
{
    char command_line[SOFTWARE_RING_BUFFER_SIZE] = {0};
    uint16_t cmd_pos = 0;
    char received_byte;

    // Read bytes until newline
    while (RingBuffer_Read(&uart2_rxRingBuffer, &received_byte))
    {
        if (cmd_pos < sizeof(command_line) - 1)
        {
            command_line[cmd_pos++] = received_byte;
            if (received_byte == '\n')
            {
                command_line[cmd_pos] = '\0';  // Null-terminate the line

                // Enqueue the full command line as a single entry
                if (commandQueue.count < COMMAND_BUFFER_SIZE)
                {
                    strncpy(commandQueue.buffer[commandQueue.tail], command_line, COMMAND_LENGTH - 1);
                    commandQueue.buffer[commandQueue.tail][COMMAND_LENGTH - 1] = '\0';
                    commandQueue.tail = (commandQueue.tail + 1) % COMMAND_BUFFER_SIZE;
                    commandQueue.count++;
                }

                break;  // Exit after a full line
            }
        }
    }
}


/**
 * @brief  process_full_command_app_uart process and enqueue complete commands received over the app UART.
 *
 * Drains bytes from the `uart3_rxRingBuffer` into a temporary buffer until a newline
 * character (`'\n'`) is encountered or the buffer is full.  The collected line is
 * then null-terminated and split into tokens using `strtok()` with `':'` as the
 * delimiter.  Each token is copied into the global `commandQueue` (up to
 * `COMMAND_LENGTH-1` characters) if there is space available.
 *
 * @note   Uses `SOFTWARE_RING_BUFFER_SIZE` for the temporary line buffer size.
 * @note   Tokens retain no trailing newline or delimiter characters.
 * @note   `commandQueue` must be large enough to hold all expected tokens;
 *         excess tokens are dropped silently if `commandQueue.count` reaches
 *         `COMMAND_BUFFER_SIZE`.
 */
void process_full_command_app_uart(void)
{
    char command_line[SOFTWARE_RING_BUFFER_SIZE] = {0};
    uint16_t cmd_pos = 0;
    char received_byte;

    while (RingBuffer_Read(&uart3_rxRingBuffer, &received_byte))
    {
        if (cmd_pos < sizeof(command_line) - 1)
        {
            command_line[cmd_pos++] = received_byte;
            if (received_byte == '\n')
            {
                command_line[cmd_pos] = '\0';
                break;
            }
        }
    }

    char *command = strtok(command_line, ":");
    while (command != NULL)
    {
        if (commandQueue.count < COMMAND_BUFFER_SIZE)
        {
            strncpy(commandQueue.buffer[commandQueue.tail], command, COMMAND_LENGTH - 1);
            commandQueue.buffer[commandQueue.tail][COMMAND_LENGTH - 1] = '\0';
            commandQueue.tail = (commandQueue.tail + 1) % COMMAND_BUFFER_SIZE;
            commandQueue.count++;
        }
        command = strtok(NULL, ":");
    }
}

/**
 * @brief  Transmits an array of values over UART in a human-readable, bracketed list.
 *         Formats each element as decimal, hexadecimal, or ASCII and sends it immediately
 *         using the blocking printToDebugUartBlocking() call.
 *
 * @param  data         Pointer to the first element of the array to transmit.
 *                      Interpret as uint8_t*, uint16_t*, or uint32_t* depending on dataSize.
 * @param  numElements  Number of elements in the array.
 * @param  dataSize     Size in bytes of each element:
 *                      - 1 ⇒ uint8_t
 *                      - 2 ⇒ uint16_t
 *                      - 4 ⇒ uint32_t
 * @param  format       OutputFormat enum specifying how to represent each element:
 *                      - OUTPUT_FORMAT_DECIMAL
 *                      - OUTPUT_FORMAT_HEX
 *                      - OUTPUT_FORMAT_ASCII
 *
 * @note   This implementation uses printToDebugUartBlocking(), which waits for each
 *         chunk to complete before returning.  It prints a leading “[”, then each value
 *         separated by commas, then a closing “]\r\n”.
 * @note   The temporary buffer size is UART_TX_MESSAGE_SIZE; longer formatted strings
 *         will be truncated.
 */
void print_ArrayToUART_Out(const void *data,
                           uint16_t numElements,
                           uint8_t dataSize,
                           OutputFormat format)
{
    static char buffer[UART_TX_MESSAGE_SIZE];  // formatting scratch pad

    // opening bracket
    printToDebugUartBlocking("[");

    for (uint16_t i = 0; i < numElements; i++)
    {
        // 8-bit
        if (dataSize == 1)
        {
            unsigned int v = ((uint8_t *)data)[i];
            switch (format)
            {
                case OUTPUT_FORMAT_DECIMAL:
                    snprintf(buffer, sizeof(buffer), "%u", v);
                    break;
                case OUTPUT_FORMAT_HEX:
                    snprintf(buffer, sizeof(buffer), "%02X", v);
                    break;
                case OUTPUT_FORMAT_ASCII:
                    snprintf(buffer, sizeof(buffer), "%c", (char)v);
                    break;
            }
        }
        // 16-bit
        else if (dataSize == 2)
        {
            unsigned int v = ((uint16_t *)data)[i];
            switch (format)
            {
                case OUTPUT_FORMAT_DECIMAL:
                    snprintf(buffer, sizeof(buffer), "%u", v);      // use %u, cast to unsigned int
                    break;
                case OUTPUT_FORMAT_HEX:
                    snprintf(buffer, sizeof(buffer), "%04X", v);    // 4-digit hex
                    break;
                case OUTPUT_FORMAT_ASCII:
                    snprintf(buffer, sizeof(buffer), "%c", (char)v);
                    break;
            }
        }
        // 32-bit
        else if (dataSize == 4)
        {
            unsigned long v = ((uint32_t *)data)[i];
            switch (format)
            {
                case OUTPUT_FORMAT_DECIMAL:
                    snprintf(buffer, sizeof(buffer), "%lu", v);
                    break;
                case OUTPUT_FORMAT_HEX:
                    snprintf(buffer, sizeof(buffer), "%08lX", v);
                    break;
                case OUTPUT_FORMAT_ASCII:
                    snprintf(buffer, sizeof(buffer), "%c", (char)v);
                    break;
            }
        }
        // print element
        printToDebugUartBlocking("%s", buffer);
        // comma if not last
        if (i + 1 < numElements)
            printToDebugUartBlocking(",");
    }

    // closing bracket + newline
    printToDebugUartBlocking("]\r\n");
}








/**
 * @brief  Non-blocking, DMA-based UART transmit with message queuing.
 *         Formats a string and enqueues it into a circular buffer. If the UART
 *         is idle, initiates a DMA transfer of the first message immediately.
 *
 * @param  format  printf-style format string for the message.
 * @param  ...     Arguments matching the format specifiers in `format`.
 *
 * @note   Requires BL_DEBUG_MSG_EN to be defined for inclusion.
 * @note   Messages longer than UART_MESSAGE_SIZE-1 are truncated.
 */
void printToDebugUart(const char *format, ...)
{
	volatile char *slot = uartQueue.buffer[uartQueue.tail];        // next free slot
    va_list args;
    int len;

    va_start(args, format);
    len = vsnprintf((char *)slot, UART_TX_MESSAGE_SIZE, format, args);  // format into slot
    va_end(args);

    if (len < 0)                                           // formatting error?
        return;                                            // bail out

    // ensure null termination on overflow
    if (len >= UART_TX_BUFFER_SIZE)
        slot[UART_TX_BUFFER_SIZE - 1] = '\0';

    // --- HERE is where you store the length ---
    // clamp len to UART_MESSAGE_SIZE-1 in case of truncation
    uartQueue.length[uartQueue.tail] = (len < UART_TX_BUFFER_SIZE ? len : UART_TX_BUFFER_SIZE - 1);

    if (uartQueue.count < UART_TX_BUFFER_SIZE)                // if there’s room in the queue
    {
        uartQueue.tail = (uartQueue.tail + 1) % UART_TX_BUFFER_SIZE;  // advance tail
        uartQueue.count++;                                         // bump count

        if (txComplete)                                           // if UART is idle
        {
            txComplete = 0;                                       // mark busy
            // start DMA of the message at 'head', using stored length
            if (HAL_UART_Transmit_DMA(DebugUart, (uint8_t*)uartQueue.buffer[uartQueue.head], uartQueue.length[uartQueue.head]) != HAL_OK)
            {
                txComplete = 1;                                   // on error, back to idle
            }
        }
    }
    else
    {
        overflowCounter++;                                       // queue was full
    }
}



/* Callbacks -----------------------------------------------------------------*/
/**
 * @brief  UART receive complete callback.
 *
 * This function is called by the HAL when a DMA‐driven UART reception
 * completes (i.e. when DMA_BUFFER_SIZE bytes have been received).
 * It checks that the interrupt came from the debug UART (DebugUart),
 * pushes the new byte into a software ring buffer, and when a newline
 * (‘\n’) is seen it calls process_full_command() to parse any complete
 * command(s). Finally, it re-arms the DMA reception for the next byte(s).
 *
 * @param  huart  Pointer to the UART handle for which the callback is invoked.
 *                Must be equal to DebugUart or Debug2Uart
 *
 * @note   DMA_BUFFER_SIZE is typically 1 here for per-byte receive.
 * @note   uartx_rxRingBuffer and uartx_rxBuf are external globals.
 * @note   process_full_command() drains uart2_rxRingBuffer into your
 *         commandQueue when it sees a newline.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == DebugUart)
    {
    	// Add the received byte to the software circular buffer
        RingBuffer_Write(&uart2_rxRingBuffer, uart2_rxBuf[0]);

        // Check if the received byte is a newline, indicating end of command
        if (uart2_rxBuf[0] == '\n')
        	process_full_command_debug_uart();
    }
    else if (huart == Debug2Uart)
    {
        // e.g. ringBuffer3, uart3_rxBuf[]
        RingBuffer_Write(&uart3_rxRingBuffer, uart3_rxBuf[0]);
        if (uart3_rxBuf[0] == '\r')
            // Signal that a full command has been received
			//uint32_t tickstart = getTick();
        	process_full_command_app_uart();
        	//uint32_t tickstart1 = getTick();
        	//printToDebugUart("%ld-%ld=%ld\r\n",tickstart1, tickstart, (tickstart1-tickstart));
    }
    else
    {
        // Unexpected UART handle?
    }

    // Restart DMA reception for the next byte (automatically wraps in circular mode)
    if (HAL_UART_Receive_DMA(huart, (uint8_t*)(huart == DebugUart ? uart2_rxBuf : uart3_rxBuf), DMA_BUFFER_SIZE) != HAL_OK)
    {
        // Optionally log or handle error
    }
}

/**
 * @brief  UART DMA transmit complete callback.
 *         Dequeues the next message from the circular buffer (if any)
 *         and starts its DMA transmission. Marks the UART idle when
 *         no messages remain.
 *
 * @param  huart  Pointer to the UART handle for which the DMA TX just completed.
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) // callback invoked by HAL on DMA TX complete
{
    if (huart == DebugUart)                                          // only handle DebugUart events
    {
        uartQueue.head = (uartQueue.head + 1) % UART_TX_BUFFER_SIZE;    // advance head to the next message
        uartQueue.count--;                                           // decrement queued message count

        if (uartQueue.count > 0)                                     // if there are still messages queued
        {
        	uint16_t len = uartQueue.length[uartQueue.head]; // grab the length stored when enqueuing
            // start DMA TX of the next message at 'head'
            if (HAL_UART_Transmit_DMA(DebugUart, (uint8_t*)uartQueue.buffer[uartQueue.head], len) != HAL_OK)
            {
                txComplete = 1;                                      // on error, mark UART idle to avoid lock-up
            }
        }
        else
        {
            txComplete = 1;                                          // no more messages: mark UART idle
        }
    }
}
