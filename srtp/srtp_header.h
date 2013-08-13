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
 *   header->reserved = sequence;
 *   header->len = len;
 * 
 *   memcpy( &header->data, data, len );
 * 
 *   return sizeof( srtp_header_t ) + len - 1; // -1 for utilizing data entry
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

   srtp_cmd_t cmd;
   uint32_t reserved;

   uint32_t len; // end of data should equal &data + len
   uint8_t data; // &data == start of data

};


#endif // SRTP_HEADER_H

