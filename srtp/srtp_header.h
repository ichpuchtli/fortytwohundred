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
enum srtp_cmd_t {

  SYN, /// @brief  Synchronise
  SYNACK, /// @brief ??
  RST, /// @brief ??
  RSTACK, /// @brief  Reset Connection
  FIN, /// @brief Finished sending data
  FINACK  /// @brief ??

};

/**
 * @brief SRTP header packet structure

@code{.c}
void pack_srtp_syn( char* buffer, uint32_t sequence, char* data, uint32_t len ){

  srtp_header_t* header = ( srtp_header_t* ) buffer;

  header->cmd = SYN;
  header->sequence= sequence;
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

  uint8_t cmd;  /// @brief srtp_cmd_t
  uint8_t len; /// @brief length of header
  uint16_t sequence; /// @brief sequence number
  uint16_t ack; /// @brief ack number
  uint16_t checksum; /// @brief checksum

};

#ifdef __cplusplus
}
#endif

#endif // SRTP_HEADER_H

