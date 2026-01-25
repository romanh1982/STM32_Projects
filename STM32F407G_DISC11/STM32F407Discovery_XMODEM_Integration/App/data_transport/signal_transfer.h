/*
 * signal_transfer.h
 *
 *  Created on: Jun 26, 2025
 *      Author: roman
 */

#ifndef DATA_TRANSPORT_SIGNAL_TRANSFER_H_
#define DATA_TRANSPORT_SIGNAL_TRANSFER_H_



/** @brief Output data type */
typedef enum {
    DATA_TYPE_FLOAT32 	= 0,
    DATA_TYPE_UINT16 	= 1,
    DATA_TYPE_Q15 		= 2,
    DATA_TYPE_UNKNOWN 	= 3
} DataType_t;

/** @brief Output data transmission method */
typedef enum {
    TRANSFER_ASCII 		= 0,
    TRANSFER_BINARY		= 1,
    TRANSFER_UNKNOWN	= 2
} TransferMode_t;

/* Filter selection coming from host */
typedef enum {
    FILT_NONE = 0,
    FILT_FIR_LP,
    FILT_FIR_BP,
    FILT_IIR,
	FILT_MAX
} FilterType_t;

/* Signal source selection coming from host */
typedef enum {
    SIG_SRC_CALC = 0,   /* generate synthetically (your current path) */
    SIG_SRC_ADC,        /* capture from ADC/DMA (future) */
	SIG_SRC_MAX
} SignalSource_t;

/**
 * @brief  Parsed signal generation parameters from JSON with fallback.
 */
typedef struct {
    uint16_t numTones_u16;        /**< Number of tones */
    uint16_t numSamples_u16;      /**< Number of samples */
    uint32_t sampl_rate;		  /**< Sampling rate define by the host */
    uint32_t *pFreqs;             /**< Pointer to frequencies array */
    uint16_t *pAmps;              /**< Pointer to amplitudes array */
    DataType_t dataType;          /**< Data type for output (float32, uint16, q15) */
    TransferMode_t transferMode;  /**< Transfer mode (ASCII or Binary) */
    FilterType_t    filterType;   /**< FILT_NONE, FILT_FIR_LP, ... */
    SignalSource_t  sigSource; 	  /**< SIG_SRC_CALC, SIG_SRC_ADC, ... */
} JsonParsedSigGenPar_HandlType_t;



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
                         TransferMode_t transferMode);

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
                        TransferMode_t transferMode);

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
                         TransferMode_t transferMode);

#endif /* DATA_TRANSPORT_SIGNAL_TRANSFER_H_ */
