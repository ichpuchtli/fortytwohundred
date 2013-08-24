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
#include <openssl/sha.h>
#include <pthread.h>

// C++ Headers
#include <map>
#include <queue>

void send_conn_request(int fd, const struct sockaddr* addr, socklen_t dest_len);

struct Conn_t {

  pthread_t tid;

  int fifo;
  char filename[32];

  int sock;
  struct sockaddr_in addr;
  socklen_t addr_len;

};

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

  while ( 1 ) {

    n = recvfrom( conn->sock, buffer, 1024, 0,( struct sockaddr* ) &src_addr, &src_addr_len );

    // if addr is unique
    //   fifo = srtp_socket
    //   conn = fd2cnn[fifo]
    //   memcpy(conn->addr, &src_addr, src_addr_len);
    //   conn->addr_len = src_addr_len;
    //   write(fifo, buffer, n);
    //   new_conns.push(fifo);
    ( void ) n;
  }

  return NULL;
}

void* client_proxy(void* param){

  int fifo_fd = *((int*)&param);

  struct Conn_t* conn = fd2conn[fifo_fd];

  int read_end = open(conn->filename, O_RDONLY);
  // close read_end on fifo_fd

  int n;
  char buffer[1024];

  while(1) { // while(fifo_fd is open) 

    while( ( n = read(read_end, buffer, 1024)) > 0 ) {
      // n = pack_srtp_packet(mesg, 1024, buffer, n)
      sendto(conn->sock, buffer, n, 0, (struct sockaddr*) &conn->addr, sizeof(struct sockaddr_in));
    }

    if( n == 0 ) { // EOF // shutdown
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

  sprintf(filename, "tmpfifo%05d", fifo_id++);

  int errcode = mkfifo(filename, FIFO_PERMISSIONS );

  if(errcode){
    perror("_srtp_socket[mkfifo]");
    return -1;
  }

  // Read/Write here however it is expected that server will only read and
  // client only write
  int fd = open(filename, O_RDWR);

  if(fd < 0){
    perror("_srtp_socket[open]");
    unlink(filename);
    return -1;
  }

  struct Conn_t* conn = (struct Conn_t*) malloc(sizeof(struct Conn_t));

  conn->fifo = fd;
  memcpy(conn->filename, filename, 32);

  fd2conn.insert({fd, conn});

  return fd;

}

int _srtp_listen( int fifo_fd, int backlog ){

  ( void ) backlog;

  struct Conn_t* conn = fd2conn[fifo_fd];

  conn->sock = socket(AF_INET, SOCK_DGRAM, 0); // IPv4

  bind( conn->sock, ( struct sockaddr* ) &conn->addr, conn->addr_len );

  pthread_create(&conn->tid, NULL, server_proxy, (void*)( long ) fifo_fd);

  pthread_detach(conn->tid);

  return 0;
}

int _srtp_bind( int socket, const struct sockaddr* address, socklen_t address_len ){

  struct Conn_t* conn = fd2conn[socket];

  // Store address for later i.e when we actually have a socket
  memcpy( &conn->addr, address, address_len );

  conn->addr_len = address_len;

  return 0;

}

int _srtp_accept( int socket, struct sockaddr* address, socklen_t* address_len ){

  ( void ) socket;

  while ( new_conns.empty() ); // Do not need lock here since only one consumer

  int fifo = new_conns.front();
  new_conns.pop();

  struct Conn_t* conn = fd2conn[ fifo ];

  memcpy( address, &conn->addr, conn->addr_len );

  *address_len = conn->addr_len;

  return fifo;

}

int _srtp_connect( int fifo_fd, const struct sockaddr* address, socklen_t address_len ){

  int srtp_sock = socket(AF_INET, SOCK_DGRAM, 0); // IPv4 

  // send_conn_request(upd_sock, address, address_len);
  // wait for reply with unique data sink addr
  //wait_for_conn(&conn->sink_addr, &conn->sink_addr_len);

  struct Conn_t* conn = fd2conn[fifo_fd];

  memcpy(&conn->addr, address, address_len);
  conn->sock = srtp_sock;

  pthread_create(&conn->tid, NULL, client_proxy, (void*)( long ) fifo_fd);

  pthread_detach(conn->tid);

  return 0;

}

int _srtp_shutdown( int socket, int how ){

  ( void ) how;

  struct Conn_t* conn = fd2conn[socket];

  close(socket);

  (void) pthread_join(conn->tid, NULL);

  return 0;
}

void send_conn_request(int fd, const struct sockaddr* addr, socklen_t addr_len){

  char pkt_buf[64];

  sendto(fd, pkt_buf, 64, 0, addr, addr_len);

}

