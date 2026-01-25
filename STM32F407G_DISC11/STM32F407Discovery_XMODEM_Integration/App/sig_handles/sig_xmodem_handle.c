/*
 * sig_xmodem_handle.c
 *
 *  Created on: Jun 26, 2025
 *      Author: roman
 */

#include <stdint.h>
#include "sig_xmodem_handle.h"
#include "json_utils.h"
#include "signal_memory.h"
#include "signal_gen.h"
#include "board_config.h"
#include "uart_app.h"
#include "xmodem_transmitter.h"

void handle_read_GenSignal_Flex_Xmodem(const char *args)
{
    const char *cmd_id = "READ_GEN_SIG_XMD";
    write_BlueLed_PD15(GPIO_PIN_SET);

    jsmn_parser parser;
    jsmntok_t tokens[64];
    jsmn_init(&parser);

    int tok_count = jsmn_parse(&parser, args, strlen(args), tokens, sizeof(tokens)/sizeof(tokens[0]));
    if (tok_count < 1 || tokens[0].type != JSMN_OBJECT) {
        send_uart_response(cmd_id, "FAIL", "{\"error\":\"invalid_json\"}");
        return;
    }

    uint16_t numTones = 0, numSamples = 0;
    size_t parsedTones = 0, parsedAmps = 0;

    if (!json_parse_u16(args, tokens, tok_count, "num_tones", &numTones) ||
        !json_parse_u16(args, tokens, tok_count, "len", &numSamples) ||
        !json_parse_array_u32(args, tokens, tok_count, "freqs", freqs_int, MAX_TONES, &parsedTones) ||
        !json_parse_array_u16(args, tokens, tok_count, "amps", amps_int, MAX_TONES, &parsedAmps) ||
        parsedTones != numTones || parsedAmps != numTones)
    {
        send_uart_response(cmd_id, "FAIL", "{\"error\":\"missing_or_invalid_fields\"}");
        return;
    }

    SignalGen_HandleType sigSettingsHandle = {
        .numSamples_u16        = numSamples,
        .samplingRate_u32      = 1024000u,
        .dcOffset_u16           = 1650u,
        .vRef_u16               = 3300u,
        .adcMaxValue_u16        = 4095u,
        .numTones_u8            = numTones,
        .pToneFreqs_u32         = freqs_int,
        .pToneAmps_u16          = amps_int,
        .sineMethod             = SINE_METHOD_CMSIS,
        .dataType               = DATA_TYPE_UINT16,
        .pOutBuffer_f32         = NULL,
        .pOutBuffer_u16         = sigBUFFER_UNION.bufU16
    };

    write_BlueLed_PD15(GPIO_PIN_RESET); platform_delay_ms(1);
    write_BlueLed_PD15(GPIO_PIN_SET);

    SignalGen_GenerateComposite_Q15(&sigSettingsHandle);

    write_BlueLed_PD15(GPIO_PIN_RESET); platform_delay_ms(1);
    write_BlueLed_PD15(GPIO_PIN_SET);

    // === JSON HEADER PRINT ===
    printToDebugUartBlocking("{\"cmd\":\"%s\",\"status\":\"OK\",\"args\":{", cmd_id);
    printToDebugUartBlocking("\"num_tones\":%u,\"len\":%u,\"freqs\":[", numTones, numSamples);
    for (uint16_t i = 0; i < numTones; i++) {
        printToDebugUartBlocking("%lu%s", freqs_int[i], (i < numTones - 1) ? "," : "]");
    }
    printToDebugUartBlocking(",\"amps\":[");
    for (uint16_t i = 0; i < numTones; i++) {
        printToDebugUartBlocking("%u%s", amps_int[i], (i < numTones - 1) ? "," : "]");
    }
    printToDebugUartBlocking("}}\r\n");

    // === INITIATE XMODEM TRANSMISSION ===
    const uint32_t bytes_to_send = sigSettingsHandle.numSamples_u16 * sizeof(uint16_t);
    if (!xmodem_transmit_init((uint8_t *)sigSettingsHandle.pOutBuffer_u16, bytes_to_send)) {
        send_uart_response(cmd_id, "FAIL", "{\"error\":\"xmodem_init_failed\"}");
        return;
    }

    uint32_t lastTick = HAL_GetTick();
    while (xmodem_transmit_state() != XMODEM_TRANSMIT_COMPLETE &&
           xmodem_transmit_state() != XMODEM_TRANSMIT_ABORT_TRANSFER)
    {
        xmodem_transmit_process(HAL_GetTick());
        if ((HAL_GetTick() - lastTick) > HAL_MAX_DELAY) {
            break;
        }
    }

    if (xmodem_transmit_state() == XMODEM_TRANSMIT_COMPLETE) {
        send_uart_response(cmd_id, "OK", "DONE");
    } else {
        send_uart_response(cmd_id, "FAIL", "ABORT");
    }

    write_BlueLed_PD15(GPIO_PIN_RESET); platform_delay_ms(1);
}


void handle_xmodem_test(const char *args)
{
    static uint8_t test_data[128];  // one block only for test
    for (uint8_t i = 0; i < 10; i++) {
        test_data[i] = i + 1;
    }

    printToDebugUartBlocking("[DBG] Setting up callbacks...\r\n");


    printToDebugUartBlocking("[DBG] Initializing transmitter...\r\n");
    if (!xmodem_transmit_init(test_data, sizeof(test_data))) {
    	printToDebugUartBlocking("[DBG] Failed to init transmitter.\r\n");
        return;
    }

    printToDebugUartBlocking("[DBG] Begin transmitting...\r\n");
    while (xmodem_transmit_state() != XMODEM_TRANSMIT_COMPLETE && xmodem_transmit_state() != XMODEM_TRANSMIT_ABORT_TRANSFER) {
        xmodem_transmit_process(HAL_GetTick());
    }

    if (xmodem_transmit_state() == XMODEM_TRANSMIT_COMPLETE) {
    	printToDebugUartBlocking("[DBG] Transmission complete.\r\n");
    } else {
        printToDebugUartBlocking("[DBG] Transmission failed.\r\n");
    }
}
