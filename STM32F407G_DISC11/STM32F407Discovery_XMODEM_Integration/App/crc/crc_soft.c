/*
 * crc.c
 *
 *  Created on: May 30, 2025
 *      Author: WiTrLab36
 *
 *  Description:
 *      CRC-32 calculation module.
 *      Implements a software-based CRC-32 (IEEE 802.3 standard polynomial 0x04C11DB7, bit-reversed 0xEDB88320).
 *      Used for verifying data integrity in binary transmissions.
 */

#include <crc_soft.h>

/**
 * @brief Calculate the CRC-32 of a data buffer.
 *
 * This function computes the CRC-32 checksum of a given buffer using the standard
 * IEEE 802.3 polynomial (0xEDB88320, reversed representation).
 *
 * @param data Pointer to the data buffer.
 * @param length Length of the data buffer in bytes.
 * @return uint32_t The computed CRC-32 checksum.
 */
uint32_t calculate_crc32(const uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < length; i++) {
        crc ^= (uint32_t)data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }
    return ~crc;
}
