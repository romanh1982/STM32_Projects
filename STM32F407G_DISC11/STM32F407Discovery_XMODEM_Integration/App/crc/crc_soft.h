/*
 * crc.h
 *
 *  Created on: May 30, 2025
 *      Author: WiTrLab36
 *
 *  Description:
 *      Header file for CRC-32 calculation module.
 */

#ifndef CRC_SOFT_H_
#define CRC_SOFT_H_

#include <stdint.h>

/**
 * @brief Calculate the CRC-32 of a data buffer.
 *
 * @param data Pointer to the data buffer.
 * @param length Length of the data buffer in bytes.
 * @return uint32_t The computed CRC-32 checksum.
 */
uint32_t calculate_crc32(const uint8_t *data, uint32_t length);

#endif /* CRC_SOFT_H_ */
