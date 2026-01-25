/*
 * fft_function.h
 *
 *  Created on: June 2, 2025
 *      Author: Roman Heinrich
 *
 *  Description:
 *      Header file for FFT processing functions.
 *      Provides interface for generating a composite signal and performing FFT
 *      on the generated data, with results transmitted via UART.
 */

#ifndef FFT_HANDLE_H_
#define FFT_HANDLE_H_

#include <stdint.h>

/**
 * @brief  Handle JSON command to generate a composite signal and transmit FFT result (binary mode).
 *
 * @param[in] json_str  Pointer to JSON string containing generation parameters:
 *                      - "num_tones" (uint16): Number of sine components.
 *                      - "len" (uint16): Number of samples to generate (must be a power of 2).
 *                      - "freqs" (array of uint32): Frequencies in Hz.
 *                      - "amps" (array of uint16): Amplitudes in mV.
 */
void handle_read_fft(const char *json_str);
void handle_read_sig_fft(const char *json_str);

#endif /* FFT_HANDLE_H_ */

