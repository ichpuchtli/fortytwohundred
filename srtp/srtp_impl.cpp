#include "srtp_impl.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <pthread.h>

// C++ Headers
#include <map>
#include <queue>
#include <set>
#include <string>

#include "srtp_util.h"
#include "srtp_debug.h"

inline bool isValidFD(int fd);
inline bool isOpenSRTPSock(int fifo_fd);
inline bool isValidIPAddress(struct sockaddr* addr, socklen_t addr_len);

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

};

std::map<std::string, int> hash2fd;

// fifo fd -> connection structure
std::map<int, Conn_t*> fd2conn;

// Accept(pop_front) <- | fifo | fifo | fifo | <- Listen thread (push_back)
std::queue<int> new_conns;

const int FIFO_PERMISSIONS = 0666;

const int UDP_PKT_SIZE = 65535 - 20 - 8; // 2^16 - IP Header - UDP Header

void* server_proxy( void* param ){

  struct sockaddr_in src_addr;
  socklen_t src_addr_len;

  int fifo_fd = *((int*)&param);

  Conn_t* conn = fd2conn[fifo_fd];

  int n;
  char buffer[ 2048 ];

  unsigned char sha1_raw[ 20 ];
  char sha1_char[ 41 ];

  debug("[server_proxy]: listing to port %d on socket %d\n", conn->addr.sin_port, fifo_fd);

  while ( 1 ) {

    n = recv_srtp_data(conn->sock, buffer, 2048, ( struct sockaddr* ) &src_addr, &src_addr_len );

    if( n < 0 ) break; // Connection Terminated

    SHA1( ( unsigned char* ) &src_addr, src_addr_len, sha1_raw );

    for(int i = 0; i < 20; i++){
      sprintf(&sha1_char[2*i], "%x", sha1_raw[i]);
    }

    sha1_char[40] = '\0';

    debug("[server_proxy]: new datagram from %s\n", sha1_char);

    std::string shasum( ( char* ) sha1_char, ( size_t ) 40 );

    if ( hash2fd.find( shasum ) == hash2fd.end() ){

      debug("[server_proxy]: datagram was unique, creating conection\n");

      int fifo = _srtp_socket( 0, 0, 0 );

      Conn_t* accept_conn = fd2conn[ fifo ];

      memcpy(&accept_conn->addr, &src_addr, src_addr_len);

      accept_conn->addr_len = src_addr_len;

      write(fifo, buffer, n);

      new_conns.push(fifo);

      hash2fd.insert( {shasum, fifo} );

    }else{

      int fifo = hash2fd[ shasum ];

      write( fifo, buffer, n );

    }

    debug("[server_proxy]: datagram contained %d bytes\n", n);

  }

  debug( "[server_proxy]: server shutting down\n" );

  return NULL;
}

void* client_proxy(void* param){

  int fifo_fd = *((int*)&param);

  struct Conn_t* conn = fd2conn[fifo_fd];

  int n, sent;
  char buffer[2048];

  while( ( n = read(fifo_fd, buffer, 2048)) > 0 ) {

    debug( "[client_proxy]: sending %d bytes\n", n );

     sent = send_srtp_data(conn->sock, buffer, n, (struct sockaddr*) &conn->addr, sizeof(struct sockaddr_in));

     if ( sent < 0 ) {
       break;
     }

  }

  debug( "[client_proxy]: client proxy shutting down\n" );

  return NULL;
}

int _srtp_socket( int domain, int type, int protocol ){

  // suppress unused warnings
  (void) domain;
  (void) type;
  (void) protocol;

  static int fifo_id = 0;

  char filename[32];

  int errcode;

  do {

    sprintf(filename, "tmpfifo%05d", fifo_id++);

    errcode = mkfifo(filename, FIFO_PERMISSIONS );

  } while(errcode);

  // Read/Write here however it is expected that server will only read and
  // client only write
  int fd = open(filename, O_RDWR);

  if(fd < 0){
    perror("srtp_socket[open]");
    unlink(filename);
    return -1;
  }

  Conn_t* conn = new Conn_t;

  conn->fifo = fd;
  memcpy(conn->filename, filename, 32);

  debug("[socket]: new srtp socket %d -> %s\n", conn->fifo, conn->filename );

  fd2conn.insert({fd, conn});

  return fd;

}

int _srtp_listen( int fifo_fd, int backlog ){

  ( void ) backlog;

  if( ! isOpenSRTPSock(fifo_fd) ){
    debug( "[listen]: invalid SRTP socket\n" );
    return -1;
  }

  struct Conn_t* conn = fd2conn[fifo_fd];

  conn->sock = socket(AF_INET, SOCK_DGRAM, 0); // IPv4

  // Unbound Listen
  if( !isValidIPAddress((struct sockaddr*) &conn->addr, conn->addr_len) ){
    debug( "[listen]: invalid ip address\n" );
    return -1;
  }

  bind( conn->sock, ( struct sockaddr* ) &conn->addr, conn->addr_len );

  pthread_create(&conn->tid, NULL, server_proxy, (void*)( long ) fifo_fd);

  debug("[listen]: detaching worker thread\n");

  pthread_detach(conn->tid);

  return 0;
}

int _srtp_bind( int socket, const struct sockaddr* address, socklen_t address_len ){

  if( ! isOpenSRTPSock(socket) ){
    debug( "[bind]: invalid SRTP socket\n" );
    return -1;
  }

  if( ! isValidIPAddress((struct sockaddr*) address, address_len) ){
    debug( "[bind]: invalid ip address\n" );
    return -1;
  }

  struct Conn_t* conn = fd2conn[socket];

  // Store address for later i.e when we actually have a socket
  memcpy( &conn->addr, address, address_len );

  conn->addr_len = address_len;

  return 0;

}

int _srtp_accept( int socket, struct sockaddr* address, socklen_t* address_len ){

  if( ! isOpenSRTPSock(socket) ){
    debug( "[accept]: invalid SRTP socket\n" );
    return -1;
  }

  while ( new_conns.empty() ){} // Do not need lock here since only one consumer

  int fifo = new_conns.front();
  new_conns.pop();

  struct Conn_t* conn = fd2conn[ fifo ];

  if(address && address_len){

    memcpy( address, &conn->addr, conn->addr_len );

    *address_len = conn->addr_len;

  }

  debug("[accept]: accepted new connection from xxx\n");

  return fifo;

}

int _srtp_connect( int fifo_fd, const struct sockaddr* address, socklen_t address_len ){

  if( ! isOpenSRTPSock(fifo_fd) ){
    debug( "[connect]: invalid SRTP socket\n" );
    return -1;
  }

  if( ! isValidIPAddress((struct sockaddr*) address, address_len) ){
    debug( "[connect]: invalid IP address\n" );
    return -1;
  }

  int srtp_sock = socket(AF_INET, SOCK_DGRAM, 0); // IPv4

  debug("[connect]: establishing connection to xxx...\n");

  struct Conn_t* conn = fd2conn[fifo_fd];

  memcpy(&conn->addr, address, address_len);
  conn->sock = srtp_sock;

  (void) establish_conn( srtp_sock , address, address_len);

  debug("[connect]: Done.\n");

  pthread_create(&conn->tid, NULL, client_proxy, (void*)( long ) fifo_fd);

  debug("[connect]: detaching client worker thread\n");

  pthread_detach(conn->tid);

  return 0;

}

int _srtp_close(int socket, int how){

  if( ! isOpenSRTPSock(socket) ){
    return -1;
  }

  debug("[close]: closing socket %d\n", socket);

  sleep(1); // yielding does not work for some reason, must sleep instead

  close(socket);

  Conn_t* conn = fd2conn[socket];

  if(conn->tid){
    (void) shutdown_conn(conn->sock, 0);
    close(conn->sock);
    pthread_cancel(conn->tid);
    pthread_join(conn->tid, NULL);
  }

  unlink(conn->filename);

  fd2conn->erase(socket);
  
  delete conn;
  
  debug("[close]: socket %d closed\n", socket);

  return 0;
}

inline bool isValidFD(int fd){

  struct stat statbuf;

  int errcode = fstat(fd, &statbuf);

  return (errcode == 0);

}

inline bool isOpenSRTPSock(int fifo_fd){

  if( !isValidFD(fifo_fd) ) {
    return false;
  }

  // fifo_fd found in map
  return fd2conn.find( fifo_fd ) != fd2conn.end();

}

inline bool isValidIPAddress(struct sockaddr* addr, socklen_t addr_len){

  return addr && (addr_len > 0);

}
