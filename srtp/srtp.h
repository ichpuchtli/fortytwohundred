#ifndef _SRTP_H_
#define _SRTP_H_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

extern int srtp_socket( int domain, int type, int protocol );

extern int srtp_listen( int socket, int backlog );

extern int srtp_bind( int socket, const struct sockaddr* address, socklen_t address_len );

extern int srtp_accept( int socket, struct sockaddr* address, socklen_t* address_len );

extern int srtp_connect( int socket, const struct sockaddr* address, socklen_t address_len );

extern int srtp_shutdown( int socket, int how );

extern int srtp_send( int socket, const void* message, size_t length, int flags );

extern int srtp_recv( int socket, void* buffer, size_t length, int flags );

#endif // _SRTP_H_
