#ifndef _SRTP_IMPL_H_
#define _SRTP_IMPL_H_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

extern int _srtp_socket( int domain, int type, int protocol );

extern int _srtp_listen( int socket, int backlog );

extern int _srtp_bind( int socket, const struct sockaddr* address, socklen_t address_len );

extern int _srtp_accept( int socket, struct sockaddr* address, socklen_t* address_len );

extern int _srtp_connect( int socket, const struct sockaddr* address, socklen_t address_len );

extern int _srtp_shutdown( int socket, int how );

extern int _srtp_send( int socket, const void* message, size_t length, int flags );

extern int _srtp_recv( int socket, void* buffer, size_t length, int flags );

#endif // _SRTP_IMPL_H_
