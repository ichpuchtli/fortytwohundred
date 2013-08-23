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

void send_conn_request(int fd, const struct sockaddr* addr, socklen_t dest_len);
  
struct ClientConn {

  // worker thread
  pthread_t tid;
  
  // Populated on socket
  int fifo_fd;
  char filename[32];
  
  // udp socket fd
  int sock_fd;
  
  // server address
  struct sockaddr_in server_addr;
  
  // data sink address returned by server
  struct sockaddr_in sink_addr;

};

// int fifo_fd -> client connection infomation
std::map<int, struct ClientConn*> fd2conn;

const int FIFO_PERMISSIONS = 0666;

const int UDP_PKT_SIZE = 65535 - 20 - 8; // 2^16 - IP Header - UDP Header

void* client_proxy(void* param){
  
  int fifo_fd = *((int*)&param);
  
  struct ClientConn* conn = fd2conn[fifo_fd];
  
  int read_end = open(conn->filename, O_RDONLY);
  // close read_end on fifo_fd
  
  int n;
  char buffer[1024];
  
  while(1) { // while(fifo_fd is open) 
    
    while( ( n = read(read_end, buffer, 1024)) > 0 ) {
      // n = pack_srtp_packet(mesg, 1024, buffer, n)
      sendto(conn->sock_fd, buffer, n, 0, (struct sockaddr*) &conn->sink_addr, sizeof(struct sockaddr_in));
    }
    
    if( n == 0 ) { // EOF // shutdown
      // send FIN and wait for FINACK
      close(conn->fifo_fd);
      unlink(conn->filename);
      close(conn->sock_fd);
      break;
    }
    
  }  
  
  return NULL;
}

// TODO map fds to fifofilename
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
  
  struct ClientConn* conn = (struct ClientConn*) malloc(sizeof(struct ClientConn));
  
  conn->fifo_fd = fd;
  memcpy(conn->filename, filename, 32);
  
  fd2conn.insert({fd, conn});
  
  return fd;
  
}

int _srtp_listen( int socket, int backlog ){
  
  // detach event loop thread
  return 0;
}

int _srtp_bind( int socket, const struct sockaddr* address, socklen_t address_len ){
  // only necessary for server usally called prior to listen
  
  // id = hash(address)
  // id2fd.insert(id, socket);
  return 0;
  
}

int _srtp_accept( int socket, struct sockaddr* address, socklen_t* address_len ){
  
  // wait/block for new fifo_fd in ready_connections_queue
  // return fifo_fd
  return 0;

}

int _srtp_connect( int fifo_fd, const struct sockaddr* address, socklen_t address_len ){
  
  int srtp_sock = socket(AF_INET, SOCK_DGRAM, 0); // IPv4 
  
  // send_conn_request(upd_sock, address, address_len);
  // wait for reply with unique data sink addr
  //wait_for_conn(&conn->sink_addr, &conn->sink_addr_len);
  
  struct ClientConn* conn = fd2conn[fifo_fd];
  
  memcpy(&conn->server_addr, address, address_len);
  conn->sock_fd = srtp_sock;
  
  pthread_create(&conn->tid, NULL, client_proxy, (void*) fifo_fd);
  
  pthread_detach(conn->tid);
  
  return 0;

}

int _srtp_shutdown( int socket, int how ){
  
  struct ClientConn* conn = fd2conn[socket];
  
  close(socket);
  
  (void) pthread_join(conn->tid, NULL);
  
  return 0;
}

int _srtp_send( int socket, const void* message, size_t length, int flags ){

  // write to fifo  
  // identical to write(fd, message, length)
  
  return 0;
}

int _srtp_recv( int socket, void* buffer, size_t length, int flags ){
  
  // read from fifo
  // identical to read(fd, buffer, length)
  return 0;
}

void send_conn_request(int fd, const struct sockaddr* addr, socklen_t addr_len){
  
  char pkt_buf[64];
  
  sendto(fd, pkt_buf, 64, 0, addr, addr_len);
  
}