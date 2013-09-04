/**
 * @file srtp_util.h
 * @brief internal srtp utilites
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
#ifndef SRTP_UTIL_H
#define SRTP_UTIL_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <map>
#include <queue>
#include <set>
#include <string>

#include "srtp_header.h"

/// @brief connect attempt timeout, ms
const int CONNECT_TIMEOUT = 5000;

/// @brief ack-wait base timeout, ms
const int RETRY_TIMEOUT_BASE = 50;

/// @brief failed ack-wait time increase, percent
const int RETRY_TIMEOUT_SCALER = 50;

/// @brief ack-wait max timeout, ms
const int RETRY_TIMEOUT_MAX = 1000;

/// @brief go-back-n ARQ protocol's "n" (# packets buffered)
const int ARQN = 16;

/// @brief maximum allowed packet size
const int PAYLOAD_MAXSIZE = 1024;

/* ----------------------------------------------------------------------------
 * The following macros are designed to operate on a character buffer
 * of at least PKT_MAXSIZE bytes, containing an SRTP packet at position 0.
 */

/// @brief SRTP header size
#define PKT_HEADERSIZE sizeof(struct srtp_header_t)
/// @brief SRTP max packet size
#define PKT_MAXSIZE (PKT_HEADERSIZE+PAYLOAD_MAXSIZE)

/// @brief Check if a packet's Command bitmask includes a certain command. 
#define PKT_HASCMD(buf,command) (((srtp_header_t *)buf)->cmd && command)
/// @brief Check if a packet's Command bitmask includes SYN
#define PKT_SYN(buf) PKT_HASCMD(buf, SYN)
/// @brief Check if a packet's Command bitmask includes ACK
#define PKT_ACK(buf) PKT_HASCMD(buf, ACK)
/// @brief Check if a packet's Command bitmask includes FIN
#define PKT_FIN(buf) PKT_HASCMD(buf, FIN)
/// @brief Check if a packet's Command bitmask includes RST
#define PKT_RST(buf) PKT_HASCMD(buf, RST)

/// @brief Get a packet's Command
#define PKT_GETCMD(buf) (((srtp_header_t *)buf)->cmd)
/// @brief Get a packet's Length
#define PKT_GETLEN(buf) (((srtp_header_t *)buf)->len)
/// @brief Get a packet's Sequence Number
#define PKT_GETSEQ(buf) (((srtp_header_t *)buf)->seq)
/// @brief Get a packet's Acknowledgement Number
#define PKT_GETACK(buf) (((srtp_header_t *)buf)->ack)
/// @brief Get a packet's Command Info Number
#define PKT_GETCMDINFO(buf) (((srtp_header_t *)buf)->cmdinfo)

/// @brief Set a packet's Command
#define PKT_SETCMD(buf, command) (((srtp_header_t *)buf)->cmd = command)
/// @brief Set a packet's Sequence Number
#define PKT_SETSEQ(buf, seqnum) (((srtp_header_t *)buf)->seq = seqnum)
/// @brief Set a packet's Acknowledgement Number
#define PKT_SETACK(buf, acknum) (((srtp_header_t *)buf)->ack = acknum)
/// @brief Set a packet's Payload Length. WARNING - NO ERROR CHECKING
#define PKT_SETLEN(buf, length) (((srtp_header_t *)buf)->len = length)
/// @brief Set a packet's Command Info Number.
#define PKT_SETCMDINFO(buf, info) (((srtp_header_t *)buf)->cmdinfo = info)

/// @brief Fetch a packet's Payload Pointer
#define PKT_PAYLOADPTR(buf) ((char*)buf + PKT_HEADERSIZE)
/// @brief Get a packet's Payload Length
#define PKT_PAYLOADLEN(buf) (((srtp_header_t *)buf)->len)

/// @brief Zero the entire packet
#define PKT_ZEROPAYLOAD(buf) memset(buf, '\0', PKT_MAXSIZE)
/// @brief Zero a packet's Payload
#define PKT_ZERO(buf) \
do { \
  memset(PKT_PAYLOADPTR(buf), '\0', PAYLOAD_MAXSIZE); \
  PKT_SETLEN(buf, 0); \
} while (0)

/// @brief Copy one packet over another
#define PKT_COPYTO(dest, src) memcpy(dest, src, (size_t)PKT_MAXSIZE)

/// @brief Check if a packet length is invalid
#define PKT_INVALID_LEN(len) (len > PAYLOAD_MAXSIZE)
/// @brief Check if a packet command is invalid
#define PKT_INVALID_CMD(cmd) (cmd > (SYN|ACK|FIN|RST|RDY))
/// @brief Check if a packet is invalid. Validate buffer length and command
///  bitmask
#define PKT_INVALID(buf) ( PKT_INVALID_LEN(PKT_GETLEN(buf)) || \
                           PKT_INVALID_CMD(PKT_GETCMD(buf)) )

// Shutdown condition 
//TODO need to expose to application, or just use ints with documented meanings
enum SHUTDOWN_CONDITIONS {

  ABNORMAL_TERMINATION,
  FATAL_CLIENT_ERROR,
  FATAL_SERVER_ERROR,
  NORMAL_TERMINATION
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief established a connection with a peer on a UDP socket 
 *
 * @note this function will NOT block indefinitely, a timeout will occur
 * if a connection can not be established before CONNECTION_ATTEMPT_TIMEOUT
 *
 * @param sock and UDP socket
 * @param addr destination address
 * @param addr_len destination address structure length
 *
 * @return 0 is returned upon successful a connection, -1 in the case of a
 * timeout
 */
int establish_conn(int sock, const struct sockaddr* addr, socklen_t addr_len);

/**
 * @brief shuts both directions of the connection down and gracefully releases 
 * any internal resources
 *
 * @param sock the UDP socket used by the peer
 * @param how a field for specifying the reason for the shutdown
 *
 * @return upon successful shutdown 0 shall be returned, -1 otherwise
 */
int shutdown_conn(int sock, int how);

/**
 * @brief receives incoming data on the UDP socket \em sock only returning
 * when their is sufficient data to be conveyed, this method will partially
 * organise connection establishments and shutdowns.
 *
 * @note not returning on until valid data is obtained presents a few challengers
 * such as maintaining a list of active data queues for all incoming destinations
 *
 * @param sock the UDP socket to recv on
 * @param buffer a buffer to place any data, only used if \em len > 0
 * @param len the amount of allocated memory for the buffer
 * @param addr the src address of the message
 * @param addr_len the src address structure length
 *
 * @return the amount of data in bytes available to be read from \em buffer, this value
 * should always be < len, -1 is returned when a connection is broken
 */
int recv_srtp_data(int sock, char* buffer, size_t len, struct sockaddr* addr, socklen_t* addr_len);

/**
 * @brief ensures outgoing data is sent to the destination address, this function
 * may send many UDP datagrams to ensure all data is received on the destination
 * host
 *
 * @param sock the UDP socket to be used by the peer
 * @param data a pointer to the data buffer
 * @param len the amount of data in that buffer
 * @param addr the destination address
 * @param addr_len the destination address structure size
 *
 *
 * @return not clear as yet but it may return the number of bytes successfully sent, -1 
 * in the case of a connection problem
 */
int send_srtp_data(int sock, char* data, size_t len, const struct sockaddr* addr, socklen_t addr_len);

/* ----------------------------------------------------------------------------
 * The functions beloware are utilities for the above methods. 
 * They are exposed in the header for unit tests.
 */

/**
 * @brief Convert a packet command bitmask to a human readable string.
 *
 * @param cmd the SRTP command bitmask
 * @param s the string buffer to print into. Recommend 32 characters
 */
void srtp_cmdstr(int cmd, char* s);

/**
 * @brief Convert a packet header and src/dest to a human readable string.
 *
 * @param cmd the SRTP command bitmask
 * @param remote the remote socket address
 * @param local the local socket address
 * @param direction SEND or RECV
 * @param s the string buffer to print into. Recommend 128 characters
 */
void pkt_str(char* buf, struct sockaddr_in remote, struct sockaddr_in local, int direction, char* s);

/**
 * @brief Validate a packet; print error messages if it's invalid
 *
 * @param buf a buffer containing an SRTP packet
 *
 * @return 0 for a valid packet, >0 otherwise
 */
int pkt_invalid(char* buf);

/**
 * @brief Write a payload into a buffer containing an srtp packet.
 *
 * @param buf a buffer containing an SRTP packet
 * @param payload a buffer containing a payload for the packet
 * @param len size of the payload, in bytes
 *
 * @return len for a successful write, -1 if payload could not be copied
 */
int pkt_set_payload(char* buf, char* payload, uint16_t len);

/**
 * @brief Construct an srtp packet from a new header and payload
 *
 * @param buf a buffer containing an SRTP packet
 * @param cmd the SRTP command bitmask
 * @param len size of the payload, in bytes
 * @param seq the SRTP sequence number
 * @param ack the SRTP ack number
 * @param cmdinfo the command info number
 * @param payload a buffer containing a payload for the packet
 *
 * @return len for a successful write, -1 if packet could not be written
 */
 int pkt_pack(char* buf, uint8_t cmd, uint16_t len, uint16_t seq, uint16_t ack, uint8_t cmdinfo, char* payload);


#ifdef __cplusplus
}
#endif

/**
 * @brief connection structure
 */
struct Conn_t {

  pthread_t tid; /// @brief the thread id of the worker thread (unsigned long)

  int fifo; /// @brief the file descriptor of the fifo
  char filename[32]; /// @brief the filename of the fifo

  int sock; /// @brief a UDP socket

  struct sockaddr_in addr; /// @brief a general purpose address structure
  socklen_t addr_len; /// @brief the address structure size
  
  unsigned int sequence; /// @brief the current sequence number

};

/* ----------------------------------------------------------------------------
 * GLOBALS
 */

/// @brief hash(addr) -> fifo fd
extern std::map<std::string, int> hash2fd;

/// @brief fifo fd -> connection structure
extern std::map<int, Conn_t*> fd2conn;

/// @brief this queue contains fifo fd's for new incoming connections
extern std::queue<int> new_conns;

/// @brief this switch enables/disables lightweight debug messages
extern bool srtp_packet_debug;

#endif // SRTP_UTIL_H
