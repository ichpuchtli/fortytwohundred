#include "srtp.h"

int srtp_socket( int domain, int type, int protocol ){
  return socket( domain, type, protocol );
}

int srtp_listen( int socket, int backlog ) {
  return listen( socket, backlog );
}

int srtp_bind( int socket, const struct sockaddr* address, socklen_t address_len ){
  return bind( socket, address, address_len );
}

int srtp_accept( int socket, struct sockaddr* address, socklen_t* address_len ){
  return accept( socket, address, address_len );
}

int srtp_connect( int socket, const struct sockaddr* address, socklen_t address_len ){
  return connect( socket, address, address_len );
}

int srtp_shutdown( int socket, int how ){
  return shutdown( socket, how );
}

int srtp_getpeername( int socket, struct sockaddr* address, socklen_t* address_len ){
  return getpeername( socket, address, address_len );
}

int srtp_send( int socket, const void* message, size_t length, int flags ){
  return send( socket, message, length, flags );
}

int srtp_recv( int socket, void* buffer, size_t length, int flags ){
  return recv( socket, buffer, length, flags );
}
