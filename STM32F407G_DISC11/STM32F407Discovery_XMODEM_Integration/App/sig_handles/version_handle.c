/*
 * version_handle.c
 *
 *  Created on: Jun 26, 2025
 *      Author: roman
 */


#include "version_handle.h"
#include "uart_app.h"
#include "version.h"

void handle_read_fw(const char *args)
{
    printToDebugUartBlocking("{\"cmd\":\"READ_FW\",\"status\":\"OK\",\"data\":\"%s\"}\r\n", SYSTEM_VERSION_STR);
}

void handle_read_hw(const char *args)
{
    printToDebugUartBlocking("{\"cmd\":\"READ_HW\",\"status\":\"OK\",\"data\":\"%s\"}\r\n", "STM32407DISCOVERY_MB998_C-01");
}

void handle_read_ser(const char *args)
{
    printToDebugUartBlocking("{\"cmd\":\"READ_SER\",\"status\":\"OK\",\"data\":\"%s\"}\r\n", "STM32407DISCOVERY#001");
}
