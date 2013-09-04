/**
 * @file srtp_header.h
 * @brief internal srtp header definitions
 * @author Sam Macpherson
 * @author Chris Ponticello
 *
 * Copyright 2013  Sam Macpherson, Chris Ponticello
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef SRTP_HEADER_H
#define SRTP_HEADER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Official SRTP commands
 */
#define SYN 0x01 /// @brief  Synchronise
#define ACK 0x02 /// @brief Acknowledge
#define FIN 0x04 /// @brief Finished sending data
#define RST 0x08 /// @brief  Reset Connection
#define RDY 0x10 /// @brief  Ready for data

/**
 * @brief Packet directions
 */
#define SEND 0
#define RECV 1

/**
 * @brief SRTP header packet structure

@code{.c}
void pack_srtp_syn( char* buffer, uint32_t sequence, char* data, uint32_t len ){

  srtp_header_t* header = ( srtp_header_t* ) buffer;

  header->cmd = SYN;
  header->seq = sequence;
  header->len = sizeof(srtp_header_t);

  memcpy( buffer + header->len, data, len );

  return header->len + len;
}

char buffer[ udp_max_size ];

int packet_len = pack_srtp_syn( buffer, 1, data, len );

sendto( sock, bufffer, packet_len );
@endcode

 */
struct srtp_header_t {
  /// @brief length of payload
  uint16_t len;
  /// @brief sequence number
  uint16_t seq;
  /// @brief acknowledgement number
  uint16_t ack;
  /// @brief command info number
  uint8_t cmdinfo;
  /// @brief srtp command bitmask
  uint8_t cmd;
};

#ifdef __cplusplus
}
#endif

#endif // SRTP_HEADER_H

