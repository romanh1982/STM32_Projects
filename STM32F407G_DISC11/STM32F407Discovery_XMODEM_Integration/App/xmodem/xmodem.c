#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "xmodem.h"
#include "board_config.h"

bool xmodem_calculate_crc(const uint8_t *data, const uint32_t size, uint16_t *result)
{
	uint32_t start_time = platform_get_time_ms();
	write_OrangeLed_PD13(GPIO_PIN_SET);

	uint16_t crc    = 0x0;
	uint32_t count  = size;
	bool     status = false;
	uint8_t  i      = 0;

	if (0 != data && 0 != result)
	{
		   status = true;

	   while (0 < count--)
	   {
		  crc = crc ^ (uint16_t) *data << 8;
			  data++;
		  i = 8;

		  do
		  {
		  if (0x8000 & crc)
		  {
			 crc = crc << 1 ^ 0x1021;
		  }
		  else
		  {
			 crc = crc << 1;
		  }

		  }
		  while (0 < --i);

	   }

	   *result = crc;
   }
	write_OrangeLed_PD13(GPIO_PIN_RESET);
	uint32_t stop_time = platform_get_time_ms();
	printf("crc calc duration %lu - %lu = %lu ms \r\n", stop_time, start_time, (stop_time - start_time));

	return status;
}

bool xmodem_verify_packet(const xmodem_packet_t packet, uint8_t expected_packet_id)
{
    bool     status         = false;
    bool     crc_status     = false;
    uint16_t calculated_crc = 0;

    // Calculate CRC on the received data
    crc_status = xmodem_calculate_crc(packet.data, XMODEM_BLOCK_SIZE, &calculated_crc);

    // Reconstruct CRC from packet (MSB first)
    uint16_t received_crc = ((uint16_t)packet.crcMSB << 8) | packet.crcLSB;

    if (packet.preamble == SOH &&
        packet.id == expected_packet_id &&
        packet.id_complement == (uint8_t)(0xFF - packet.id) &&
        crc_status &&
        calculated_crc == received_crc)
    {
        status = true;
    }

    return status;
}




