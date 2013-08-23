#include "srtp.h"
#include "srtp_impl.h"

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
