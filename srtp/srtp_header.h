#ifdef SRTP_HEADER_H
#define SRTP_HEADER_H

#include <stdint.h>

/**
 * @brief Official SRTP commands
 */
enum srtp_cmd_t {

  SYN, //  Synchronise
  SYNACK,
  RST, //  Reset Connection
  RSTACK, //  Reset Connection
  FIN, //  Finished sending data
  FINACK 

};

/**
 * @brief SRTP header packet structure
 * @code{.c}
 *
 * void pack_srtp_syn( char* buffer, uint32_t sequence, char* data, uint32_t len ){
 * 
 *   srtp_header_t* header = ( srtp_header_t* ) buffer;
 * 
 *   header->cmd = SYN;
 *   header->sequence= sequence;
 *   header->len = sizeof(srtp_header_t);
 * 
 *   memcpy( buffer + header->len, data, len );
 * 
 *   return header->len + len;
 * }
 * 
 * char buffer[ udp_max_size ];
 * 
 * int packet_len = pack_srtp_syn( buffer, 1, data, len );
 * 
 * sendto( sock, bufffer, packet_len );
 * @endcode
 *
 */
struct srtp_header_t {

  uint8_t cmd;  // srtp_cmd_t
  uint8_t len; // length of header
  uint16_t sequence; // sequence number
  uint16_t ack; // ack number
  uint16_t checksum; // checksum

};


#endif // SRTP_HEADER_H

