#include "uart_app.h"            // for print_ArrayToUART_Out()
#include <stdint.h>

/**
 * @brief  Run a suite of tests for print_ArrayToUART_Out().
 *         For each combination of data size and format:
 *           1) prints “Expected:” line
 *           2) invokes print_ArrayToUART_Out() to show “Actual:”
 */
void test_print_ArrayToUART_Out(void)
{
    // --- 8-bit tests ---
    const uint8_t arr8_num[]    = { 1,   2,   3   };
    const uint8_t arr8_hex[]    = { 0x0A, 0xAB, 0xFF };
    const uint8_t arr8_ascii[]  = { 'X',  'Y',  'Z'  };

    printToDebugUartBlocking("\r\n--- 8-bit (dataSize=1) ---\r\n");

    // Decimal
    printToDebugUartBlocking("Expected: [1,2,3]\r\nActual:   ");
    print_ArrayToUART_Out(arr8_num,    3, 1, OUTPUT_FORMAT_DECIMAL);

    // Hex
    printToDebugUartBlocking("Expected: [0A,AB,FF]\r\nActual:   ");
    print_ArrayToUART_Out(arr8_hex,    3, 1, OUTPUT_FORMAT_HEX);

    // ASCII
    printToDebugUartBlocking("Expected: [X,Y,Z]\r\nActual:   ");
    print_ArrayToUART_Out(arr8_ascii,  3, 1, OUTPUT_FORMAT_ASCII);


    // --- 16-bit tests ---
    const uint16_t arr16_num[]   = { 1000, 2000, 3000 };
    const uint16_t arr16_hex[]   = { 0x1234, 0xABCD, 0xFFFF };
    const uint16_t arr16_ascii[] = { 'A',    'B',    'C'    };

    printToDebugUartBlocking("\r\n--- 16-bit (dataSize=2) ---\r\n");

    // Decimal
    printToDebugUartBlocking("Expected: [1000,2000,3000]\r\nActual:      ");
    print_ArrayToUART_Out(arr16_num,   3, 2, OUTPUT_FORMAT_DECIMAL);

    // Hex
    printToDebugUartBlocking("Expected: [1234,ABCD,FFFF]\r\nActual:      ");
    print_ArrayToUART_Out(arr16_hex,   3, 2, OUTPUT_FORMAT_HEX);

    // ASCII
    printToDebugUartBlocking("Expected: [A,B,C]\r\nActual:      ");
    print_ArrayToUART_Out(arr16_ascii, 3, 2, OUTPUT_FORMAT_ASCII);


    // --- 32-bit tests ---
    const uint32_t arr32_num[]   = { 100000, 200000, 300000 };
    const uint32_t arr32_hex[]   = { 0x12345678, 0xABCDEF01, 0xFFFFFFFF };
    const uint32_t arr32_ascii[] = { 'x',         'y',         'z'          };

    printToDebugUartBlocking("\r\n--- 32-bit (dataSize=4) ---\r\n");

    // Decimal
    printToDebugUartBlocking("Expected: [100000,200000,300000]\r\nActual:      ");
    print_ArrayToUART_Out(arr32_num,   3, 4, OUTPUT_FORMAT_DECIMAL);

    // Hex
    printToDebugUartBlocking("Expected: [12345678,ABCDEF01,FFFFFFFF]\r\nActual:      ");
    print_ArrayToUART_Out(arr32_hex,   3, 4, OUTPUT_FORMAT_HEX);

    // ASCII
    printToDebugUartBlocking("Expected: [x,y,z]\r\nActual:      ");
    print_ArrayToUART_Out(arr32_ascii, 3, 4, OUTPUT_FORMAT_ASCII);

    printToDebugUartBlocking("\r\n*** Test complete ***\r\n");
}
