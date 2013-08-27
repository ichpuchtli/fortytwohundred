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


//#define SRTP_DEBUG

#include "srtp_debug.h"

void send_conn_request(int fd, const struct sockaddr* addr, socklen_t dest_len);

inline bool isValidFD(int fd);
inline bool isOpenSRTPSock(int fifo_fd);
inline bool isValidIPAddress(struct sockaddr* addr, socklen_t addr_len);

struct Conn_t {

  pthread_t tid;

  int fifo;
  char filename[32];

  int sock;
  struct sockaddr_in addr;
  socklen_t addr_len;

};

std::map<std::string, int> hash2fd;

// fifo fd -> connection structure
std::map<int, struct Conn_t*> fd2conn;

// Accept(pop_front) <- | fifo | fifo | fifo | <- Listen thread (push_back)
std::queue<int> new_conns;

const int FIFO_PERMISSIONS = 0666;

const int UDP_PKT_SIZE = 65535 - 20 - 8; // 2^16 - IP Header - UDP Header

void* server_proxy( void* param ){

  struct sockaddr_in src_addr;
  socklen_t src_addr_len;

  int fifo_fd = *((int*)&param);

  struct Conn_t* conn = fd2conn[fifo_fd];

  int n;
  char buffer[ 1024 ];

  unsigned char sha1_buf[ 40 ];

  debug("[server_proxy]: listing to port %d on socket %d\n", conn->addr.sin_port, conn->sock );

  while ( 1 ) {

    n = recvfrom( conn->sock, buffer, 1024, 0,( struct sockaddr* ) &src_addr, &src_addr_len );

    if( n <= 0 ) break;

    SHA1( ( unsigned char* ) &src_addr, src_addr_len, sha1_buf );

    std::string shasum( ( char* ) sha1_buf, ( size_t ) src_addr_len );

    if ( hash2fd.find( shasum ) == hash2fd.end() ){

      int fifo = _srtp_socket( 0, 0, 0 );

      struct Conn_t* conn = fd2conn[ fifo ];

      memcpy(&conn->addr, &src_addr, src_addr_len);

      conn->addr_len = src_addr_len;

      write(fifo, buffer, n);

      new_conns.push(fifo);

      hash2fd.insert( {shasum, fifo} );

    }else{

      int fifo = hash2fd[ shasum ];

      write( fifo, buffer, n );

    }

  }

  return NULL;
}

void* client_proxy(void* param){

  int fifo_fd = *((int*)&param);

  struct Conn_t* conn = fd2conn[fifo_fd];

  int read_end = fifo_fd;

  //int read_end = open(conn->filename, O_RDONLY);
  // close read_end on fifo_fd

  int n;
  char buffer[1024];

  while(1) { // while(fifo_fd is open) 

    while( ( n = read(read_end, buffer, 1024)) > 0 ) {
      // n = pack_srtp_packet(mesg, 1024, buffer, n)
      sendto(conn->sock, buffer, n, 0, (struct sockaddr*) &conn->addr, sizeof(struct sockaddr_in));
    }

    if( n < 0 ) { // EOF // shutdown
      debug("[client_proxy]: EOF closing files\n");
      // send FIN and wait for FINACK
      close(conn->fifo);
      unlink(conn->filename);
      close(conn->sock);
      break;
    }

  }

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

  struct Conn_t* conn = (struct Conn_t*) calloc(sizeof(struct Conn_t), sizeof(char));

  conn->fifo = fd;
  memcpy(conn->filename, filename, 32);

  debug("[socket]: new srtp socket %d -> %s\n", conn->fifo, conn->filename );

  fd2conn.insert({fd, conn});

  return fd;

}

int _srtp_listen( int fifo_fd, int backlog ){

  ( void ) backlog;

  if( ! isOpenSRTPSock(fifo_fd) ){
    return -1;
  }

  struct Conn_t* conn = fd2conn[fifo_fd];

  conn->sock = socket(AF_INET, SOCK_DGRAM, 0); // IPv4

  // Unbound Listen
  if( !isValidIPAddress((struct sockaddr*) &conn->addr, conn->addr_len) ){
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
    return -1;
  }

  if( ! isValidIPAddress((struct sockaddr*) address, address_len) ){
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
    return -1;
  }

  while ( new_conns.empty() ); // Do not need lock here since only one consumer

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
    return -1;
  }

  if( ! isValidIPAddress((struct sockaddr*) address, address_len) ){
    return -1;
  }

  int srtp_sock = socket(AF_INET, SOCK_DGRAM, 0); // IPv4

  debug("[connect]: establishing connection to xxx...\n");

  // send_conn_request(upd_sock, address, address_len);
  // wait for reply with unique data sink addr
  //wait_for_conn(&conn->sink_addr, &conn->sink_addr_len);

  struct Conn_t* conn = fd2conn[fifo_fd];

  memcpy(&conn->addr, address, address_len);
  conn->sock = srtp_sock;

  debug("[connect]: Done.\n");

  pthread_create(&conn->tid, NULL, client_proxy, (void*)( long ) fifo_fd);

  debug("[connect]: detaching client worker thread\n");

  pthread_detach(conn->tid);

  return 0;

}

int _srtp_shutdown( int socket, int how ){

  ( void ) how;

  debug("[shutdown]: shutting down socket %d due to %d\n", socket, how);

  if( ! isOpenSRTPSock(socket) ){
    return -1;
  }

  struct Conn_t* conn = fd2conn[socket];

  close(socket);

  debug("[shutdown]: joining with worker thread\n");

  (void) pthread_join(conn->tid, NULL);

  debug("[shutdown]: shutting down\n");

  return 0;
}

void send_conn_request(int fd, const struct sockaddr* addr, socklen_t addr_len){

  char pkt_buf[64];

  sendto(fd, pkt_buf, 64, 0, addr, addr_len);

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
