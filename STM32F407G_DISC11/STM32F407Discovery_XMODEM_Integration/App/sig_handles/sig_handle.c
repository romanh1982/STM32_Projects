/*
 * sig_handle.c
 *
 *  Created on: Jun 26, 2025
 *      Author: roman
 */

#include "sig_handle.h"
#include "signal_gen.h"
#include "board_config.h"
#include "signal_memory.h"
#include "uart_app.h"
#include "json_utils.h"
#include "fft_utils.h"
#include "signal_transfer.h"
#include "signal_config_parser.h"


/**
 * @brief  Handle JSON command to generate a composite signal, scale it to float32, and send ASCII numbers in JSON or binary.
 *
 * @param[in] json_str  Pointer to JSON string containing generation parameters:
 *                      - "num_tones" (uint16): Number of sine components.
 *                      - "len" (uint16): Number of samples to generate (will be clipped to max buffer length).
 *                      - "freqs" (array of uint32): Frequencies in Hz.
 *                      - "amps" (array of uint16): Amplitudes in mV.
 *                      - "data_type" (optional, enum): Output data type.
 *                      - "transfer" (optional, enum): Output transfer method (ASCII/BINARY).
 */
void handle_read_scaled_signal(const char *json_str)
{
    write_BlueLed_PD15(GPIO_PIN_SET);

    JsonParsedSigGenPar_HandlType_t config;

    // --- Parse and Validate JSON Parameters; fill config struct (may apply defaults for missing fields) ---
    if (parse_and_validate_signal_config(json_str, "READ_SCALED_SIG", &config) != 0) {
        write_BlueLed_PD15(GPIO_PIN_RESET);
        return;  // Early exit on error
    }

    // --- Setup signal generation handle; adapt if you want more fields configurable ---
    SignalGen_HandleType sigSettingsHandle = {
        .numSamples_u16        = config.numSamples_u16,
        .samplingRate_u32      = config.sampl_rate,
        .dcOffset_u16          = 1650U,        // Hardcoded DC offset [mV]
        .vRef_u16              = 3300U,        // Hardcoded reference voltage [mV]
        .adcMaxValue_u16       = 4095U,        // Hardcoded ADC max (12-bit)
        .numTones_u8           = (uint8_t)config.numTones_u16,
        .pToneFreqs_u32        = config.pFreqs,
        .pToneAmps_u16         = config.pAmps,
        .sineMethod            = SINE_METHOD_CMSIS,
        .dataType              = DATA_TYPE_FLOAT32,  // Always float32 output for this handler
        .pOutBuffer_f32        = sigBUFFER_UNION.bufF32,   // Main float output buffer
        .pOutBuffer_u16        = NULL               // Not used
    };

    write_BlueLed_PD15(GPIO_PIN_RESET);
    platform_delay_ms(1U);

    // --- Generate Composite Signal in float32 ---
    write_BlueLed_PD15(GPIO_PIN_SET);
    SignalGen_GenerateComposite(&sigSettingsHandle);
    write_BlueLed_PD15(GPIO_PIN_RESET);
    platform_delay_ms(1U);

    // Convert mV to [-1, 1] based on your Vref:
    float32_t mv_to_unit = 1.0f / sigSettingsHandle.vRef_u16;  // i.e., 1/3300 for 3.3V
    // Convert mV → unit scale (V/V)
    arm_scale_f32(sigBUFFER_UNION.bufF32, mv_to_unit, sigBUFFER_UNION.bufF32, sigSettingsHandle.numSamples_u16);

    // --- Send response using unified JSON/ASCII or binary protocol ---
    write_BlueLed_PD15(GPIO_PIN_SET);
    //send_signal_response("READ_SCALED_SIG", &config, sigBUFFER_UNION.bufF32, sigSettingsHandle.numSamples_u16, config.dataType, config.transferMode);
    send_signal_header("READ_SCALED_SIG", &config, sigBUFFER_UNION.bufF32, sigSettingsHandle.numSamples_u16, config.dataType, config.transferMode);
    send_signal_payload(sigBUFFER_UNION.bufF32, sigSettingsHandle.numSamples_u16, config.dataType, config.transferMode);
    write_BlueLed_PD15(GPIO_PIN_RESET);
    platform_delay_ms(1U);
}
