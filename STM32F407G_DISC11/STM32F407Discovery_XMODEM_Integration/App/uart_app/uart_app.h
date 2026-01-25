#ifndef UART_APP_H_
#define UART_APP_H_

/* Private includes ----------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "board_config.h"

/* Exported constants --------------------------------------------------------*/
#define DMA_BUFFER_SIZE                     1


#define DEBUG_UART_PRINT_BUFFER_SIZE 128

/* Public Defines -----------------------------------------------------------*/
#define UART_RX_BUFFER_SIZE                	5      	// Adjust this based on max. length of the command message
#define UART_RX_RING_BUFFER_SIZE           	128     // Ring buffer size
#define SOFTWARE_RING_BUFFER_SIZE 			512  	// Software ring buffer size
#define DMA_BUFFER_SIZE 					1   	// 1-byte DMA buffer

#define COMMAND_BUFFER_SIZE 				4  // Number of commands the buffer can hold
#define COMMAND_LENGTH 						512       // Max length of a single command

/* Export Enumerations -------------------------------------------------------*/
// Enum to specify the data format (Hex, Decimal, ASCII)
typedef enum {
    OUTPUT_FORMAT_HEX,
    OUTPUT_FORMAT_DECIMAL,
    OUTPUT_FORMAT_ASCII
} OutputFormat;

/* Exported types ------------------------------------------------------------*/
// Command queue buffer
typedef struct {
    char buffer[COMMAND_BUFFER_SIZE][COMMAND_LENGTH];  // 2D array to store multiple commands
    uint8_t head;  // Points to the next command to be executed
    uint8_t tail;  // Points to the next empty slot
    uint8_t count; // Number of commands in the buffer
} CommandBuffer;

// UART receive ring buffer
typedef struct {
    volatile char buffer[SOFTWARE_RING_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} UART_RingBuffer;

/* External variables --------------------------------------------------------*/

extern volatile char uart2_rxBuf[DMA_BUFFER_SIZE];
extern UART_RingBuffer uart2_rxRingBuffer;
extern volatile char uart3_rxBuf[DMA_BUFFER_SIZE];
extern UART_RingBuffer uart3_rxRingBuffer;

extern CommandBuffer commandQueue;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef *DebugUart;

/* Exported functions prototypes ---------------------------------------------*/
int __io_putchar(int ch);
void printToDebugUartBlocking(char *format, ...);
void printToDebug2UartBlocking(char *format, ...);
void send_uart_response(const char *cmd, const char *status, const char *payload_fmt, ...);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void process_full_command(void);
void print_ArrayToUART_Out(const void *data, uint16_t numElements, uint8_t dataSize, OutputFormat format);
int RingBuffer_Read(UART_RingBuffer *ringBuffer, char *data);

#endif /* UART_APP_H_ */

