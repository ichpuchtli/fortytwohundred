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
#include <pthread.h>
#include <sys/time.h> // for struct timeval & gettimeofday

#include "srtp_debug.h"

ssize_t srtp_read(int sock, char* buffer, size_t length);
ssize_t srtp_write(int sock, char* buffer, size_t length);
ssize_t srtp_sendto(int sock, char* message, size_t length, int flags, struct sockaddr* dest_addr, socklen_t dest_len);

int handle_data_pkt( srtp_header_t* head, Conn_t* conn );
int handle_cmd_pkt(int sock, srtp_header_t* head, Conn_t* conn );

void send_ack_to( int sock, unsigned int seq, struct sockaddr* addr, socklen_t addr_len );

std::map<long, int> hash2fd;

std::map<int, Conn_t*> fd2conn;

std::queue<int> new_conns;

bool srtp_packet_debug = 0;

int establish_conn(int sock, const struct sockaddr* addr, socklen_t addr_len){

  // Don't need these since we connect'ed the UDP socket earlier in srtp_impl
  (void) addr;
  (void) addr_len;

  char buff[PKT_HEADERSIZE];

  struct timeval t1, t2;
  unsigned long dt = 0;
  unsigned long total_msec = 0;

  while(1) {

ESTABLISH_SEND_SYN:

    PKT_SETCMD(buff, SYN);
    PKT_SETSEQ(buff, 0);
    PKT_SETLEN(buff, 0);

    (void) srtp_write(sock, buff, PKT_HEADERSIZE); // assume write wont block

    gettimeofday(&t2, NULL); // Stamp t2 with microseconds

    while(1) { 

      if(srtp_read(sock, buff, PKT_HEADERSIZE) == PKT_HEADERSIZE){
        // Got back a packet, lets see if it is the ack we expected
        if(PKT_ACK(buff) && (PKT_GETACK(buff) == 0)){
          break;
        }
      }

      gettimeofday(&t1, NULL);

      dt = (t2.tv_sec - t1.tv_sec) * 1000000U + (t2.tv_usec - t1.tv_usec); 

      total_msec += dt/1000;

      // Total timeout
      if( total_msec > (unsigned long)CONNECT_TIMEOUT) {
        return -1;
      }

      // Re-send timeout
      if(dt > (RETRY_TIMEOUT_BASE * 1000)){
        goto ESTABLISH_SEND_SYN;
      }

      // for cpu sake
      usleep(10000);

    }

  }

  PKT_SETCMD(buff, ACK);
  PKT_SETSEQ(buff, 0); //does this need to be 0 or 1: 0 initially, see http://goo.gl/B8Yvea
  PKT_SETACK(buff, 0);
  PKT_SETLEN(buff, 0);

  (void) srtp_write(sock, buff, PKT_HEADERSIZE); // assume write wont block

  return 0;

}

int shutdown_conn(int sock, int how){
  (void) sock;
  (void) how;

  return 0;
}


int handle_data_pkt( srtp_header_t* head, Conn_t* conn ){
  if (conn == NULL) {
    fprintf(stderr, "handle_data_pkt() received NULL as a connection\n");
    return 1;
  }
  int fifo = conn->fifo;
  char* buffer = ( char* ) head + sizeof( srtp_header_t );

  if(conn->sequence == head->seq){

    // if we block here we may miss UDP packets, the alternative however is not better
    write(fifo, buffer + sizeof( srtp_header_t ), head->len );

    conn->sequence++;

    // check if more messages can be sent in order now

  } else {

    char* mesg_buf = new char[ head->len ];

    memcpy(mesg_buf, buffer + sizeof(srtp_header_t), head->len);

    std::list<Mesg_t*>::iterator pos = conn->inbox.end();

    for (std::list<Mesg_t*>::iterator it = conn->inbox.begin();

      it != conn->inbox.end(); it++ ){

      if ( ( *it )->seq > head->seq ){
        pos = it;
        break;
      }

    }

    Mesg_t* mesg = new Mesg_t;

    mesg->mesg = mesg_buf;
    mesg->len = head->len;
    mesg->seq = head->seq;

    // Insert mesg before pos unless pos is end() then insert will act as push back
    conn->inbox.insert(pos, mesg);

  }

  return 0;
}

int handle_cmd_pkt(int sock, srtp_header_t* head, Conn_t* conn ){

  // Connection Request
  if ( ( head->cmd & SYN ) && ( head->seq == 0 ) ) {
    // send syn ack
  }

  // Teardown Request
  if ( ( head->cmd & FIN ) ) {
    //sendto fin ack
  }

  return 0;
}

int recv_srtp_data(int sock, char* buffer, size_t len, struct sockaddr* addr, socklen_t* addr_len){

  int n;

  srtp_header_t* header = ( srtp_header_t* ) buffer;

  while ( 1 ) {

    n = recvfrom( sock, buffer, len, 0, addr, addr_len );

    // Would block, i.e idle
    if ( n < 0 ) {

      // Perfect time to check all connection inboxes to see if data can be passed
      // down fifo's in order
      continue;
    }

    // this will create a fifo & conn_t if it doesn't exist
    int fifo = hash2fd[((struct sockaddr_in *)addr)->sin_addr.s_addr];

    Conn_t* conn = fd2conn[ fifo ];
    if (conn == NULL) {
        fifo = _srtp_socket(0, 0, 0);
        conn = fd2conn[fifo];
        memcpy(&conn->addr, addr, *addr_len);
        conn->addr_len = len;
        hash2fd.insert( {((struct sockaddr_in *)addr)->sin_addr.s_addr, fifo});
    }

    if ( header->len > 0 ) { // Data Packet

      ( void ) handle_data_pkt( header, conn );

      send_ack_to(sock, header->seq, addr, *addr_len); // Send syn ack

    } else { // Command Packet

      ( void ) handle_cmd_pkt(sock, header, conn );

    }

  }

  return 0;

}

int send_srtp_data(int sock, char* data, size_t len, const struct sockaddr* addr, socklen_t addr_len ) {

  char buff[PKT_HEADERSIZE];
  
  struct timeval t1, t2;
  unsigned long dt = 0;
  unsigned long total_msec = 0;
  
  int seq = 0; // TODO proper sequence tally
  int sent = 0;
  
  while(1) {
    
SENDTO_SEND_SYN:

    PKT_SETCMD(buff, SYN);
    PKT_SETSEQ(buff, seq); //TODO how will we count sequence nums??
    PKT_SETLEN(buff, len);
    
    sent = srtp_write(sock, data, len); // assume write wont block
    
    if(sent < 0){
      return -1;
    }
    
    gettimeofday(&t2, NULL); // Stamp t2 with microseconds
    
    while(1) { 
      
      if(srtp_read(sock, buff, PKT_HEADERSIZE) == PKT_HEADERSIZE){
        // Got back a packet, lets see if it is the ack we expected
        if(PKT_ACK(buff) && (PKT_GETACK(buff) == seq)){
          break;
        }
      }
      
      gettimeofday(&t1, NULL);
      
      dt = (t2.tv_sec - t1.tv_sec) * 1000000U + (t2.tv_usec - t1.tv_usec); 
      
      total_msec += dt/1000;
      
      // Re-send timeout
      if(dt > (RETRY_TIMEOUT_BASE * 1000)){
        goto SENDTO_SEND_SYN;
      }      
      
      // for cpu sake
      usleep(10000);
      
    }
    
  }

  return sent;
  
}

// ----------------------------------------------------------------------------

void srtp_cmdstr(int cmd, char* s) {
  s[0] = '\0';
  if (cmd&RST) {
    if (s[0]!='\0') strcat(s, "+");
    strcat(s, "RST");
  }
  if (cmd&SYN) {
    if (s[0]!='\0') strcat(s, "+");
    strcat(s, "SYN");
  }
  if (cmd&FIN) {
    if (s[0]!='\0') strcat(s, "+");
    strcat(s, "FIN");
  }
  if (cmd&RDY) {
    if (s[0]!='\0') strcat(s, "+");
    strcat(s, "RDY");
  }
  if (cmd&ACK) {
    if (s[0]!='\0') strcat(s, "+");
    strcat(s, "ACK");
  }
}

void pkt_str(char* buf, struct sockaddr_in remote, struct sockaddr_in local, int direction, char* s) {
  if (srtp_packet_debug) {
    //hh:mm:ss xxx.xxx.xxx:xxxxx [<|>] SYN+ACK [<|>] local:xxxxx | seq=n ack=n [len=n] [code=n]
  }
}

int pkt_invalid(char* buf) {
  if (PKT_INVALID_LEN(PKT_GETLEN(buf))) {
    debug("[pkt_invalid] PKT_INVALID_LEN(%u)\n", PKT_GETLEN(buf));
    return 1;
  }
  if (PKT_INVALID_CMD(PKT_GETCMD(buf))) {
    debug("[pkt_invalid] PKT_INVALID_CMD(%u)\n", PKT_GETCMD(buf));
    return 1;
  }
  return 0;
}

int pkt_set_payload(char* buf, char* payload, uint16_t len) {
  if (PKT_INVALID_LEN(len)) {
    debug("[pkt_set_payload] PKT_INVALID_LEN(%u)\n", len);
    return -1;
  }
  memcpy(PKT_PAYLOADPTR(buf), payload, (size_t)len);
  PKT_SETLEN(buf, len);
  
  return len;
}

int pkt_pack(char* buf, uint8_t cmd, uint16_t len, uint16_t seq, uint16_t ack, uint8_t cmdinfo, char* payload) {
  if (PKT_INVALID_CMD(cmd)) {
    debug("[pkt_set_payload] PKT_INVALID_LEN(%u)\n", len);
    return -1;
  }
  PKT_SETCMD(buf, cmd);
  PKT_SETSEQ(buf, seq);
  PKT_SETACK(buf, ack);
  PKT_SETCMDINFO(buf, cmdinfo);

  return pkt_set_payload(buf, payload, len);
}


ssize_t srtp_write(int sock, char* buffer, size_t length){

  ssize_t sent = 0;

  do{
    sent += write(sock, &buffer[sent], length - sent);
  } while(sent > 0 && (size_t) sent < length);

  return sent;
}

ssize_t srtp_read(int sock, char* buffer, size_t length){

  ssize_t recvd = 0;

  do{
    recvd += read(sock, &buffer[recvd], length - recvd);
  } while(recvd > 0 && (size_t) recvd < length);

  return recvd;

}

ssize_t srtp_sendto(int sock, char* message, size_t length, int flags, struct sockaddr* dest_addr, socklen_t dest_len) {

  ssize_t sent = 0;

  do{
    sent += sendto(sock, &message[sent], length - sent, flags, dest_addr, dest_len);
  } while(sent > 0 && (size_t) sent < length);

  return sent;

}

void send_ack_to( int sock, unsigned int seq, struct sockaddr* addr, socklen_t addr_len ){

  srtp_header_t head;

  head.cmd = ACK;
  head.len = 0;
  head.seq = seq;

  ( void ) srtp_sendto( sock, ( char* ) &head, sizeof( srtp_header_t ), 0, addr, addr_len );

}


inline bool isValidFD(int fd);
inline bool isOpenSRTPSock(int fifo_fd);
inline bool isValidIPAddress(struct sockaddr* addr, socklen_t addr_len);

const int FIFO_PERMISSIONS = 0666;

const int UDP_PKT_SIZE = 65535 - 20 - 8; // 2^16 - IP Header - UDP Header

void* server_proxy( void* param ){

  struct sockaddr_in src_addr;
  socklen_t src_addr_len;

  int fifo_fd = *((int*)&param);

  Conn_t* conn = fd2conn[fifo_fd];

  int n;
  char buffer[ 2048 ];

  debug("[server_proxy]: listing to port %d on socket %d\n", conn->addr.sin_port, fifo_fd);

  while ( 1 ) {

    n = recv_srtp_data(conn->sock, buffer, 2048, ( struct sockaddr* ) &src_addr, &src_addr_len );

    if( n < 0 ) break; // Connection Terminated

    debug("[server_proxy]: new datagram from %s\n", src_addr.sin_addr.s_addr);

    if ( hash2fd.find( src_addr.sin_addr.s_addr ) == hash2fd.end() ){

      debug("[server_proxy]: datagram was unique, creating conection\n");

      int fifo = _srtp_socket( 0, 0, 0 );

      Conn_t* accept_conn = fd2conn[ fifo ];
      memcpy(&accept_conn->addr, &src_addr, src_addr_len);
      accept_conn->addr_len = src_addr_len;

      write(fifo, buffer, n);

      new_conns.push(fifo);

      hash2fd.insert( {src_addr.sin_addr.s_addr, fifo} );

    }else{

      int fifo = hash2fd[ src_addr.sin_addr.s_addr ];

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

  int bret = bind( conn->sock, ( struct sockaddr* ) &conn->addr, conn->addr_len );
  if (bret == -1) {
    debug("[listen]: binding failed\n");
    return bret;
  }

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

  while ( new_conns.empty() ){ 
    usleep(20); // Do not need lock here since only one consumer
  } 
  int fifo = new_conns.front();
  new_conns.pop();

  struct Conn_t* conn = fd2conn[ fifo ];

  if(address && address_len){

    memcpy( address, &conn->addr, conn->addr_len );

    conn->addr_len = *address_len;

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

  int cret = connect(srtp_sock, address, address_len);
  if (cret == -1) {
    debug("[connect]: connect failed\n");
    return cret;
  }

  int flags = fcntl(srtp_sock, F_GETFL, 0);
  flags |= O_NONBLOCK;
  fcntl(srtp_sock, F_SETFL, flags);

  debug("[connect]: establishing connection to xxx...\n");

  struct Conn_t* conn = fd2conn[fifo_fd];

  memcpy(&conn->addr, address, address_len);
  conn->sock = srtp_sock;

  if(establish_conn( srtp_sock , address, address_len)){
    debug("[connect]: failed.\n");
    close(srtp_sock);
    return -1;
  }

  debug("[connect]: Done.\n");

  pthread_create(&conn->tid, NULL, client_proxy, (void*)( long ) fifo_fd);

  debug("[connect]: detaching client worker thread\n");

  pthread_detach(conn->tid);

  return 0;

}

int _srtp_close(int socket, int how){

  ( void ) how;

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

  fd2conn.erase(socket);

  delete conn;

  debug("[close]: socket %d closed\n", socket);

  return 0;
}

void _srtp_debug(bool enable){
  
  srtp_packet_debug = enable;
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
