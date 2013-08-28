#ifndef SRTP_UTIL_H
#define SRTP_UTIL_H

#include <sys/socket.h>

int establish_conn(int sock, const struct sockaddr* addr, socklen_t addr_len);

int shutdown_conn(int sock, int how);

int recv_srtp_data(int sock, char* buffer, size_t len, struct sockaddr* addr, socklen_t* addr_len);

int send_srtp_data(int sock, char* data, size_t len, const struct sockaddr* addr, socklen_t addr_len);

#endif // SRTP_UTIL_H
