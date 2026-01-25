/*
 * fft_function.c
 *
 *  Created on: June 2, 2025
 *      Author: Roman Heinrich
 *
 *  Description:
 *      This module generates a composite signal based on provided parameters,
 *      computes its FFT using CMSIS-DSP's arm_rfft_fast_f32, and sends
 *      the magnitude spectrum over UART in binary format.
 */

#include <fft_handle.h>
#include "signal_gen.h"
#include "board_config.h"
#include "signal_memory.h"
#include "uart_app.h"
#include "json_utils.h"
#include "fft_utils.h"
#include "signal_transfer.h"
#include "signal_config_parser.h"
#include "filter_coefficients.h"






/**
 * @brief  Handle JSON command to generate a composite signal, scale it to float32, perform FFT, and send spectrum as ASCII/Binary.
 *
 * @param[in] json_str  Pointer to JSON string containing generation parameters:
 *                      - "num_tones" (uint16): Number of sine components.
 *                      - "len" (uint16): Number of samples to generate (will be clipped to max buffer length).
 *                      - "freqs" (array of uint32): Frequencies in Hz.
 *                      - "amps" (array of uint16): Amplitudes in mV.
 *                      - "data_type" (optional, enum): Output data type.
 *                      - "transfer" (optional, enum): Output transfer method (ASCII/BINARY).
 */
void handle_read_fft(const char *json_str)
{
	write_OrangeLed_PD13(GPIO_PIN_SET);
    JsonParsedSigGenPar_HandlType_t config;
    /* --- Parse and Validate JSON Parameters and write them into config structure --- */
    if (parse_and_validate_signal_config(json_str, "READ_FFT", &config) != 0) {
        return;  // Early exit on error
    }

    uint16_t supported_length = get_supported_fft_length(config.numSamples_u16);
    bool status_len = is_valid_fft_length(supported_length);
    if(!status_len){
    	printToDebugUartBlocking("[DBG] Error : Unsupported Length\r\n");
    }
    /* --- Setup signal generation handle --- */
    SignalGen_HandleType sigSettingsHandle = {
        .numSamples_u16        = supported_length,
        .samplingRate_u32      = config.sampl_rate,
        .dcOffset_u16          = 1650U,
        .vRef_u16              = 3300U,
        .adcMaxValue_u16       = 4095U,
        .numTones_u8           = (uint8_t)config.numTones_u16,
        .pToneFreqs_u32        = config.pFreqs,
        .pToneAmps_u16         = config.pAmps,
        .sineMethod            = SINE_METHOD_CMSIS,
        .dataType              = DATA_TYPE_FLOAT32,
        .pOutBuffer_f32        = sigBUFFER_UNION.bufF32,
        .pOutBuffer_u16        = NULL
    };
    write_OrangeLed_PD13(GPIO_PIN_RESET);
    platform_delay_ms(1U);

    //***************** Generate Composite Signal (Directly as float32) ***********************************************//
    write_OrangeLed_PD13(GPIO_PIN_SET);
    SignalGen_GenerateComposite(&sigSettingsHandle);
    write_OrangeLed_PD13(GPIO_PIN_RESET);
    platform_delay_ms(1U);

    //***************** Initialize and apply FIR-FILTER ***************************************************************//
    write_OrangeLed_PD13(GPIO_PIN_SET);
    arm_fir_instance_f32 fir;
    arm_fir_init_f32(&fir, NUM_TAPS_FIR_LP, (float32_t *)LP_FIR_COEFF, sigBUFF2, sigSettingsHandle.numSamples_u16);
    arm_fir_f32(&fir, sigBUFFER_UNION.bufF32, sigBUFFER_UNION.bufF32, sigSettingsHandle.numSamples_u16);
    write_OrangeLed_PD13(GPIO_PIN_RESET);
    platform_delay_ms(1U);

    //***************** Remove DC (center to 0) and normalize to [-1, 1] **********************************************//
    const float32_t adcMidpoint = 2048.0f;
    const float32_t scaleFactor = 1.0f / (adcMidpoint - 1.0f);
    arm_offset_f32(sigBUFFER_UNION.bufF32, -adcMidpoint, sigBUFFER_UNION.bufF32, sigSettingsHandle.numSamples_u16);
    arm_scale_f32(sigBUFFER_UNION.bufF32, scaleFactor, sigBUFFER_UNION.bufF32, sigSettingsHandle.numSamples_u16);

    //***************** Apply Blackman window ***********************************************************************//
    write_OrangeLed_PD13(GPIO_PIN_SET);
    apply_blackman_window(sigBUFFER_UNION.bufF32, sigSettingsHandle.numSamples_u16);
    write_OrangeLed_PD13(GPIO_PIN_RESET);
    platform_delay_ms(1U);

    //***************** FFT setup and execution ***********************************************************************//
    write_OrangeLed_PD13(GPIO_PIN_SET);
    arm_rfft_fast_instance_f32 fft_instance;
    arm_status status = arm_rfft_fast_init_f32(&fft_instance, sigSettingsHandle.numSamples_u16);
    if (status != ARM_MATH_SUCCESS) {
        send_uart_response("READ_FFT", "FAIL", "{\"error\":\"fft_init_failed\"}");
        write_OrangeLed_PD13(GPIO_PIN_RESET);
        return;
    }
    arm_rfft_fast_f32(&fft_instance, sigBUFFER_UNION.bufF32, sigBUFF2, 0); // Compute FFT (real input, complex output)
    write_OrangeLed_PD13(GPIO_PIN_RESET);
    platform_delay_ms(1U);


    //***************** Compute magnitude spectrum from complex FFT ****************************************************//
    write_OrangeLed_PD13(GPIO_PIN_SET);
    arm_cmplx_mag_f32(sigBUFF2, sigBUFFER_UNION.bufF32, sigSettingsHandle.numSamples_u16 / 2);
    const float32_t coherent_gain_blackman = 1.0f / 0.42f;
    arm_scale_f32(sigBUFFER_UNION.bufF32, (coherent_gain_blackman / (sigSettingsHandle.numSamples_u16 / 4)), sigBUFF2, sigSettingsHandle.numSamples_u16);
    write_OrangeLed_PD13(GPIO_PIN_RESET);
    platform_delay_ms(1U);

    //***************** Send FFT Output as cmlx magnitude **************************************************************//
    write_OrangeLed_PD13(GPIO_PIN_SET);
    send_signal_header("READ_FFT", &config, sigBUFF2, sigSettingsHandle.numSamples_u16 / 2, config.dataType, config.transferMode);
    send_signal_payload(sigBUFF2, sigSettingsHandle.numSamples_u16 / 2, config.dataType, config.transferMode);
    write_OrangeLed_PD13(GPIO_PIN_RESET);
    platform_delay_ms(1U);
}


void handle_read_sig_fft(const char *json_str)
{
	write_OrangeLed_PD13(GPIO_PIN_SET);
    JsonParsedSigGenPar_HandlType_t config;
    /* --- Parse and Validate JSON Parameters and write them into config structure --- */
    if (parse_and_validate_signal_config(json_str, "READ_SIG_FFT", &config) != 0) {
        return;  // Early exit on error
    }

    uint16_t supported_length = get_supported_fft_length(config.numSamples_u16);
    bool status_len = is_valid_fft_length(supported_length);
    if(!status_len){
    	printToDebugUartBlocking("[DBG] Error : Unsupported Length\r\n");
    }
    /* --- Setup signal generation handle --- */
    SignalGen_HandleType sigSettingsHandle = {
        .numSamples_u16        = supported_length,
        .samplingRate_u32      = config.sampl_rate,
        .dcOffset_u16          = 1600U,
        .vRef_u16              = 3300U,
        .adcMaxValue_u16       = 4095U,
        .numTones_u8           = (uint8_t)config.numTones_u16,
        .pToneFreqs_u32        = config.pFreqs,
        .pToneAmps_u16         = config.pAmps,
        .sineMethod            = SINE_METHOD_CMSIS,
        .dataType              = DATA_TYPE_FLOAT32,
        .pOutBuffer_f32        = sigBUFFER_UNION.bufF32,
        .pOutBuffer_u16        = NULL
    };
    write_OrangeLed_PD13(GPIO_PIN_RESET);
    platform_delay_ms(1U);

    //***************** Generate Composite Signal (Directly as float32) ***********************************************//
    //***************** It takes around 55ms to generate a signal of 4096 points with 14 freq. tones and noise ********//
    write_OrangeLed_PD13(GPIO_PIN_SET);
    SignalGen_GenerateComposite(&sigSettingsHandle);
    write_OrangeLed_PD13(GPIO_PIN_RESET);
    platform_delay_ms(1U);


    //***************** Send Time-Domain Signal Unfiltered ************************************************************//
    //***************** It takes around 200ms to send 4096points x 4 = 16.4kByte + Header at 921600 Baud-Rate *********//
    write_OrangeLed_PD13(GPIO_PIN_SET);
    send_signal_header("SIG_TIME_RAW", &config, sigBUFFER_UNION.bufF32, sigSettingsHandle.numSamples_u16, config.dataType, config.transferMode);
    send_signal_payload(sigBUFFER_UNION.bufF32, sigSettingsHandle.numSamples_u16, config.dataType, config.transferMode);
    write_OrangeLed_PD13(GPIO_PIN_RESET);
    platform_delay_ms(1U);

    //***************** Initialize and apply FILTER Dependent on user selection ***************************************//
    //***************** It takes around 8.2ms to perform FIR filtering with 89-Taps on signal with 4096 points ********//
    write_OrangeLed_PD13(GPIO_PIN_SET);
    arm_fir_instance_f32 fir;
    if(config.filterType == FILT_FIR_LP){
    	arm_fir_init_f32(&fir, NUM_TAPS_FIR_LP, (float32_t *)LP_FIR_COEFF, sigBUFF2, sigSettingsHandle.numSamples_u16);
    	arm_fir_f32(&fir, sigBUFFER_UNION.bufF32, sigBUFFER_UNION.bufF32, sigSettingsHandle.numSamples_u16);
    }
    else if(config.filterType == FILT_FIR_BP){
    	arm_fir_init_f32(&fir, NUM_TAPS_FIR_BP, (float32_t *)BP_FIR_COEFF, sigBUFF2, sigSettingsHandle.numSamples_u16);
    	arm_fir_f32(&fir, sigBUFFER_UNION.bufF32, sigBUFFER_UNION.bufF32, sigSettingsHandle.numSamples_u16);
    }
    else{
    	// Do nothing. No filter.
    }

    write_OrangeLed_PD13(GPIO_PIN_RESET);
    platform_delay_ms(1U);

//    //***************** Remove DC (center to 0) and normalize to [-1, 1] **********************************************//
//    //***************** It takes around 300us to remove offset and scale the signal with 4096 points ******************//
//    write_OrangeLed_PD13(GPIO_PIN_SET);
//    const float32_t adcMidpoint = 2048.0f;
//    const float32_t scaleFactor = 1.0f / (adcMidpoint - 1.0f);
//    arm_offset_f32(sigBUFFER_UNION.bufF32, -adcMidpoint, sigBUFFER_UNION.bufF32, sigSettingsHandle.numSamples_u16);
//    arm_scale_f32(sigBUFFER_UNION.bufF32, scaleFactor, sigBUFFER_UNION.bufF32, sigSettingsHandle.numSamples_u16);
//    write_OrangeLed_PD13(GPIO_PIN_RESET);
//    platform_delay_ms(1U);

    //***************** Send Time-Domain Signal ***********************************************************************//
    //***************** It takes around 200ms to send 4096points x 4 = 16.4kByte + Header at 921600 Baud-Rate *********//
    write_OrangeLed_PD13(GPIO_PIN_SET);
    send_signal_header("SIG_TIME", &config, sigBUFFER_UNION.bufF32, sigSettingsHandle.numSamples_u16, config.dataType, config.transferMode);
    send_signal_payload(sigBUFFER_UNION.bufF32, sigSettingsHandle.numSamples_u16, config.dataType, config.transferMode);
    write_OrangeLed_PD13(GPIO_PIN_RESET);
    platform_delay_ms(1U);

    //***************** Apply Blackman window ***********************************************************************//
    //***************** It takes around 13ms to apply a blackman window on a signal of 4096 points ******************//
    write_OrangeLed_PD13(GPIO_PIN_SET);
    apply_blackman_window(sigBUFFER_UNION.bufF32, sigSettingsHandle.numSamples_u16);
    write_OrangeLed_PD13(GPIO_PIN_RESET);
    platform_delay_ms(1U);

    //***************** FFT setup and execution ***********************************************************************//
    //***************** It takes around 2ms to perform FFT on a signal of 4096 points *********************************//
    write_OrangeLed_PD13(GPIO_PIN_SET);
    arm_rfft_fast_instance_f32 fft_instance;
    arm_status status = arm_rfft_fast_init_f32(&fft_instance, sigSettingsHandle.numSamples_u16);
    if (status != ARM_MATH_SUCCESS) {
        send_uart_response("READ_FFT", "FAIL", "{\"error\":\"fft_init_failed\"}");
        write_OrangeLed_PD13(GPIO_PIN_RESET);
        return;
    }
    arm_rfft_fast_f32(&fft_instance, sigBUFFER_UNION.bufF32, sigBUFF2, 0); // Compute FFT (real input, complex output)
    write_OrangeLed_PD13(GPIO_PIN_RESET);
    platform_delay_ms(1U);

    //***************** Compute magnitude spectrum from complex FFT ****************************************************//
    //***************** It takes around 0.4ms to perform arm_cmplx_mag_f32 on a signal of 4096 points ******************//
    write_OrangeLed_PD13(GPIO_PIN_SET);
    arm_cmplx_mag_f32(sigBUFF2, sigBUFFER_UNION.bufF32, sigSettingsHandle.numSamples_u16 / 2);
    const float32_t coherent_gain_blackman = 1.0f / 0.42f;
    arm_scale_f32(sigBUFFER_UNION.bufF32, (coherent_gain_blackman / (sigSettingsHandle.numSamples_u16 / 4)), sigBUFF2, sigSettingsHandle.numSamples_u16);
    write_OrangeLed_PD13(GPIO_PIN_RESET);
    platform_delay_ms(1U);

    //***************** Send FFT Output as cmlx magnitude **************************************************************//
    //***************** It takes around 100ms to send 2048points x 4 = 8.2kByte + Header at 921600 Baud-Rate ***********//
    write_OrangeLed_PD13(GPIO_PIN_SET);
    send_signal_header("SIG_FFT", &config, sigBUFF2, sigSettingsHandle.numSamples_u16 / 2, config.dataType, config.transferMode);
    send_signal_payload(sigBUFF2, sigSettingsHandle.numSamples_u16 / 2, config.dataType, config.transferMode);
    write_OrangeLed_PD13(GPIO_PIN_RESET);
    platform_delay_ms(1U);
}




