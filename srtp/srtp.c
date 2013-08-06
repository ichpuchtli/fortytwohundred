#include "srtp.h"
#include "srtp_impl.h"

#define USE_STD_TCP

int srtp_socket( int domain, int type, int protocol ){
  #ifdef USE_STD_TCP
    return socket( domain, type, protocol );
  #else
    return _srtp_socket( domain, type, protocol );
  #endif
}

int srtp_listen( int socket, int backlog ) {
  #ifdef USE_STD_TCP
    return listen( socket, backlog );
  #else
    return _srtp_listen( socket, backlog );
  #endif
}

int srtp_bind( int socket, const struct sockaddr* address, socklen_t address_len ){
  #ifdef USE_STD_TCP
    return bind( socket, address, address_len );
  #else
    return _srtp_bind( socket, address, address_len );
  #endif
}

int srtp_accept( int socket, struct sockaddr* address, socklen_t* address_len ){
  #ifdef USE_STD_TCP
    return accept( socket, address, address_len );
  #else
    return _srtp_accept( socket, address, address_len );
  #endif
}

int srtp_connect( int socket, const struct sockaddr* address, socklen_t address_len ){
  #ifdef USE_STD_TCP
    return connect( socket, address, address_len );
  #else
    return _srtp_connect( socket, address, address_len );
  #endif
}

int srtp_shutdown( int socket, int how ){
  #ifdef USE_STD_TCP
    return shutdown( socket, how );
  #else
    return _srtp_shutdown( socket, how );
  #endif
}

int srtp_getpeername( int socket, struct sockaddr* address, socklen_t* address_len ){
  #ifdef USE_STD_TCP
    return getpeername( socket, address, address_len );
  #else
    return _srtp_getpeername( socket, address, address_len );
  #endif
}

int srtp_send( int socket, const void* message, size_t length, int flags ){
  #ifdef USE_STD_TCP
    return send( socket, message, length, flags );
  #else
    return _srtp_send( socket, message, length, flags );
  #endif
}

int srtp_recv( int socket, void* buffer, size_t length, int flags ){
  #ifdef USE_STD_TCP
    return recv( socket, buffer, length, flags );
  #else
    return _srtp_recv( socket, buffer, length, flags );
  #endif
}
