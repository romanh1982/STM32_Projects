/*
 * signal_config_parser.c
 *
 *  Created on: Jun 26, 2025
 *      Author: roman
 */

#include <stdint.h>

#include "signal_gen.h"
#include "stdbool.h"
#include "signal_memory.h"

#include "json_utils.h"
#include "uart_app.h"  // <-- For printToDebugUartBlocking()
#include "signal_config_parser.h"

/**
 * @brief  Parse and validate signal generation parameters from JSON string.
 *         Now uses integer codes for data_type and transfer mode to optimize parsing.
 *
 * @param[in]  json_str   Pointer to JSON string.
 * @param[in]  cmd_name   Command name for error reporting.
 * @param[out] config     Pointer to JsonParsedSigGenPar_HandlType_t to fill.
 * @return 0 on success, no early exits — fallback defaults on invalid fields.
 */
int32_t parse_and_validate_signal_config(
    const char *json_str,
    const char *cmd_name,
    JsonParsedSigGenPar_HandlType_t *config)
{
    jsmn_parser parser;
    jsmntok_t tokens[64U];
    jsmn_init(&parser);

    int32_t tokCount_s32 = jsmn_parse(
        &parser, json_str, (uint32_t)strlen(json_str),
        tokens, sizeof(tokens) / sizeof(tokens[0]));

    if ((tokCount_s32 < 1) || (tokens[0].type != JSMN_OBJECT)) {
        send_uart_response(cmd_name, "FAIL", "{\"error\":\"invalid_json\"}");
        return -1;
    }

    size_t parsedTones_u32 = 0U;
    size_t parsedAmps_u32  = 0U;
    JsonParseStatus_t st;
    uint32_t code_u32;
    uint16_t code_u16;

    /* --- num_tones --- */
    st = json_parse_u16(json_str, tokens, (uint32_t)tokCount_s32, "num_tones", &config->numTones_u16);
    if (st != JSON_PARSE_OK) {
        config->numTones_u16 = 1U;
        printToDebugUartBlocking("[DBG]: Warning: 'num_tones' missing/invalid. Defaulting to 1.\r\n");
    }

    /* --- len --- */
    st = json_parse_u16(json_str, tokens, (uint32_t)tokCount_s32, "len", &config->numSamples_u16);
    if (st != JSON_PARSE_OK) {
        config->numSamples_u16 = 1024U;
        printToDebugUartBlocking("[DBG]: Warning: 'len' missing/invalid. Defaulting to 1024.\r\n");
    }

    /* --- freqs --- */
    st = json_parse_array_u32(json_str, tokens, (uint32_t)tokCount_s32, "freqs",
                              freqs_int, MAX_TONES, &parsedTones_u32);
    if (st != JSON_PARSE_OK) {
        freqs_int[0]     = 10000U;
        parsedTones_u32  = 1U;
        printToDebugUartBlocking("[DBG]: Warning: 'freqs' missing/invalid. Defaulting to 10kHz.\r\n");
    }

    /* --- amps --- */
    st = json_parse_array_u16(json_str, tokens, (uint32_t)tokCount_s32, "amps",
                              amps_int, MAX_TONES, &parsedAmps_u32);
    if (st != JSON_PARSE_OK) {
        amps_int[0]     = 1000U;
        parsedAmps_u32  = 1U;
        printToDebugUartBlocking("[DBG]: Warning: 'amps' missing/invalid. Defaulting to 1000mV.\r\n");
    }

    /* --- sampl_rate --- */
    st = json_parse_u32(json_str, tokens, (uint32_t)tokCount_s32, "sampl_rate", &config->sampl_rate);
    if (st != JSON_PARSE_OK) {
        config->sampl_rate = 1024000U;
        printToDebugUartBlocking("[DBG]: Warning: 'sampl_rate' missing/invalid. Defaulting to 1024000.\r\n");
    }

    /* --- Validate array length match --- */
    if ((parsedTones_u32 != (size_t)config->numTones_u16) ||
        (parsedAmps_u32  != (size_t)config->numTones_u16)) {
        printToDebugUartBlocking("[DBG]: Warning: 'num_tones' and array lengths mismatch. Clipping to min length.\r\n");
        uint16_t minTones = (uint16_t)((parsedTones_u32 < parsedAmps_u32) ? parsedTones_u32 : parsedAmps_u32);
        config->numTones_u16 = minTones;
    }

    /* --- Clip numSamples --- */
    if (config->numSamples_u16 > MAX_SIG_LEN) {
        printToDebugUartBlocking("[DBG]: Warning: 'len'=%u exceeds max=%u. Clipping.\r\n",
                                 (uint32_t)config->numSamples_u16,
                                 (uint32_t)MAX_SIG_LEN);
        config->numSamples_u16 = MAX_SIG_LEN;
    }

    /* --- Assign frequency/amp arrays --- */
    config->pFreqs = freqs_int;
    config->pAmps  = amps_int;

    /* --- data_type --- */
    st = json_parse_u32(json_str, tokens, (uint32_t)tokCount_s32, "data_type", &code_u32);
    if (st == JSON_PARSE_OK && code_u32 <= DATA_TYPE_Q15) {
        config->dataType = (DataType_t)code_u32;
    } else {
        config->dataType = DATA_TYPE_FLOAT32;
        printToDebugUartBlocking("[DBG]: Warning: 'data_type' missing/invalid (code=%lu). Defaulting to float32.\r\n",
                                 (unsigned long)code_u32);
    }

    /* --- transfer --- */
    st = json_parse_u32(json_str, tokens, (uint32_t)tokCount_s32, "transfer", &code_u32);
    if (st == JSON_PARSE_OK && code_u32 <= TRANSFER_BINARY) {
        config->transferMode = (TransferMode_t)code_u32;
    } else {
        config->transferMode = TRANSFER_ASCII;
        printToDebugUartBlocking("[DBG]: Warning: 'transfer' missing/invalid (code=%lu). Defaulting to ASCII.\r\n",
                                 (unsigned long)code_u32);
    }

    /* --- filt_type --- */
    st = json_parse_u16(json_str, tokens, (uint32_t)tokCount_s32, "filt_type", &code_u16);
    if (st == JSON_PARSE_OK && code_u16 < (uint16_t)FILT_MAX) {
        config->filterType = (FilterType_t)code_u16;
    } else {
        config->filterType = FILT_NONE;
        printToDebugUartBlocking("[DBG]: Warning: 'filt_type' missing/invalid (code=%u). Defaulting to FILT_NONE.\r\n",
                                 (unsigned)code_u16);
    }

    /* --- sig_source --- */
    st = json_parse_u16(json_str, tokens, (uint32_t)tokCount_s32, "sig_source", &code_u16);
    if (st == JSON_PARSE_OK && code_u16 < (uint16_t)SIG_SRC_MAX) {
        config->sigSource = (SignalSource_t)code_u16;
    } else {
        config->sigSource = SIG_SRC_CALC;
        printToDebugUartBlocking("[DBG]: Warning: 'sig_source' missing/invalid (code=%u). Defaulting to SIG_SRC_CALC.\r\n",
                                 (unsigned)code_u16);
    }

    return 0;
}

