/*
 * signal_transfer.c
 *
 *  Created on: Jun 26, 2025
 *      Author: roman
 */


#include "uart_app.h"
#include "crc.h"
#include "crc_soft.h"

#include "signal_transfer.h"
#include "arm_math_include.h"	// To make float32_t known to this file


/**
 * @brief  Send signal response over UART, supporting JSON+ASCII or JSON+binary.
 * @param[in] cmd_name     Command name for the JSON (e.g., "READ_SCALED_SIG_ASCII").
 * @param[in] config       Parsed signal generation parameters.
 * @param[in] data_ptr     Pointer to signal data buffer.
 * @param[in] num_samples  Number of samples.
 * @param[in] data_type    Data type enum (uint16_t, q15_t, float32_t).
 * @param[in] binary_mode  0 = ASCII JSON array output; 1 = binary output after JSON header.
 */
void send_signal_response(const char *cmd_name,
                         const JsonParsedSigGenPar_HandlType_t *config,
                         const void *data_ptr,
                         uint16_t num_samples,
                         DataType_t data_type,
                         TransferMode_t transferMode)
{
    uint32_t data_size_bytes = 0U;

    // --- Determine element size
    switch (data_type)
    {
        case DATA_TYPE_FLOAT32: data_size_bytes = sizeof(float32_t); break;
        case DATA_TYPE_UINT16:  data_size_bytes = sizeof(uint16_t);  break;
        case DATA_TYPE_Q15:     data_size_bytes = sizeof(q15_t);     break;
        default:                data_size_bytes = sizeof(float32_t); break;
    }

    // --- Prepare and send JSON header ONLY (no data attached)
    printToDebugUartBlocking("{\"cmd\":\"%s\",\"status\":\"OK\",\"args\":{", cmd_name);
    printToDebugUartBlocking("\"num_tones\":%u,", (uint32_t)config->numTones_u16);
    printToDebugUartBlocking("\"len\":%u,", (uint32_t)num_samples);
    printToDebugUartBlocking("\"data_type\":\"%u\",", data_type);
    printToDebugUartBlocking("\"transferMode\":\"%u\"", transferMode);

    // For binary, include CRC in header, for ASCII, omit it (optional).
    if (transferMode == TRANSFER_BINARY)
    {
        uint32_t crc32 = calculate_crc32((const uint8_t *)data_ptr, num_samples * data_size_bytes);
        printToDebugUartBlocking(",\"crc\":%lu", crc32);
    }
    printToDebugUartBlocking("}}\r\n");  // single newline, end of header

    // --- Send data separately ---
    if (transferMode == TRANSFER_ASCII)
    {
        // Send data array as standalone JSON array
        printToDebugUartBlocking("{\"data\":{\"SIG1\":[");
        switch (data_type)
        {
            case DATA_TYPE_FLOAT32:
            {
                const float32_t *pData = (const float32_t *)data_ptr;
                for (uint32_t i = 0U; i < num_samples; ++i)
                    printToDebugUartBlocking("%.6f%s", pData[i], (i < (num_samples - 1U)) ? "," : "");
            } break;
            case DATA_TYPE_UINT16:
            {
                const uint16_t *pData = (const uint16_t *)data_ptr;
                for (uint32_t i = 0U; i < num_samples; ++i)
                    printToDebugUartBlocking("%u%s", pData[i], (i < (num_samples - 1U)) ? "," : "");
            } break;
            case DATA_TYPE_Q15:
            {
                const q15_t *pData = (const q15_t *)data_ptr;
                for (uint32_t i = 0U; i < num_samples; ++i)
                    printToDebugUartBlocking("%d%s", pData[i], (i < (num_samples - 1U)) ? "," : "");
            } break;
            default:
                break;
        }
        printToDebugUartBlocking("]}}\r\n");  // End of ASCII JSON data
    }
    else
    {
        // Send binary data block (no wrapper JSON)
        HAL_UART_Transmit(&huart2, (const uint8_t *)data_ptr, num_samples * data_size_bytes, HAL_MAX_DELAY);
    }
}


/**
 * @brief Send only the signal header over UART in JSON format.
 *
 * @param[in] cmd_name      Command name to embed in the response (e.g., "READ_SCALED_SIG").
 * @param[in] config        Pointer to parsed signal generation parameters.
 * @param[in] data_ptr      Pointer to the signal data (for CRC computation if binary).
 * @param[in] num_samples   Number of samples in the signal buffer.
 * @param[in] data_type     Data type of the signal buffer.
 * @param[in] transferMode  Transfer mode (ASCII or Binary).
 */
void send_signal_header(const char *cmd_name,
                        const JsonParsedSigGenPar_HandlType_t *config,
                        const void *data_ptr,
                        uint16_t num_samples,
                        DataType_t data_type,
                        TransferMode_t transferMode)
{
    uint32_t data_size_bytes = 0U;

    // Determine the byte size of each sample element
    switch (data_type)
    {
        case DATA_TYPE_FLOAT32: data_size_bytes = sizeof(float32_t); break;
        case DATA_TYPE_UINT16:  data_size_bytes = sizeof(uint16_t);  break;
        case DATA_TYPE_Q15:     data_size_bytes = sizeof(q15_t);     break;
        default:                data_size_bytes = sizeof(float32_t); break;
    }

    // --- Send JSON header
    printToDebugUartBlocking("{\"cmd\":\"%s\",\"status\":\"OK\",\"args\":{", cmd_name);
    printToDebugUartBlocking("\"num_tones\":%u,", (uint32_t)config->numTones_u16);
    printToDebugUartBlocking("\"len\":%u,", (uint32_t)num_samples);
    printToDebugUartBlocking("\"data_type\":\"%u\",", data_type);
    printToDebugUartBlocking("\"transferMode\":\"%u\"", transferMode);

    // If binary transfer, append CRC checksum
    if (transferMode == TRANSFER_BINARY)
    {
        uint32_t crc32 = calculate_crc32((const uint8_t *)data_ptr, num_samples * data_size_bytes);
        printToDebugUartBlocking(",\"crc\":%lu", crc32);
    }

    printToDebugUartBlocking("}}\r\n");  // Close JSON header
}


/**
 * @brief Send the actual signal data (as ASCII JSON array or binary block).
 *
 * @param[in] data_ptr      Pointer to the signal buffer.
 * @param[in] num_samples   Number of samples to send.
 * @param[in] data_type     Data type of the buffer elements.
 * @param[in] transferMode  Output format: ASCII JSON or binary.
 */
void send_signal_payload(const void *data_ptr,
                         uint16_t num_samples,
                         DataType_t data_type,
                         TransferMode_t transferMode)
{
    if (transferMode == TRANSFER_ASCII)
    {
        printToDebugUartBlocking("{\"data\":{\"SIG1\":[");
        switch (data_type)
        {
            case DATA_TYPE_FLOAT32:
            {
                const float32_t *pData = (const float32_t *)data_ptr;
                for (uint32_t i = 0; i < num_samples; ++i)
                    printToDebugUartBlocking("%.6f%s", pData[i], (i < num_samples - 1) ? "," : "");
                break;
            }
            case DATA_TYPE_UINT16:
            {
                const uint16_t *pData = (const uint16_t *)data_ptr;
                for (uint32_t i = 0; i < num_samples; ++i)
                    printToDebugUartBlocking("%u%s", pData[i], (i < num_samples - 1) ? "," : "");
                break;
            }
            case DATA_TYPE_Q15:
            {
                const q15_t *pData = (const q15_t *)data_ptr;
                for (uint32_t i = 0; i < num_samples; ++i)
                    printToDebugUartBlocking("%d%s", pData[i], (i < num_samples - 1) ? "," : "");
                break;
            }
            default:
                break;
        }
        printToDebugUartBlocking("]}}\r\n");  // Close JSON payload
    }
    else
    {
        // Transmit raw binary data directly
        uint32_t data_size_bytes = 0U;
        switch (data_type)
        {
            case DATA_TYPE_FLOAT32: data_size_bytes = sizeof(float32_t); break;
            case DATA_TYPE_UINT16:  data_size_bytes = sizeof(uint16_t);  break;
            case DATA_TYPE_Q15:     data_size_bytes = sizeof(q15_t);     break;
            default:                return;  // Unknown data type
        }
        HAL_UART_Transmit(&huart2, (const uint8_t *)data_ptr, num_samples * data_size_bytes, HAL_MAX_DELAY);
    }
}

