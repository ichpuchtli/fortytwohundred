#ifndef SRTP_UTIL_H
#define SRTP_UTIL_H

#include <sys/socket.h>

const int CONNECTION_ATTEMPT_TIMEOUT = 5000; // 5000 milliseconds

enum SHUTDOWN_CONDITIONS {

  ABNORMAL_TERMINATION,
  FATAL_CLIENT_ERROR,
  FATAL_SERVER_ERROR,
  NORMAL_TERMINATION
};

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

#endif // SRTP_UTIL_H
