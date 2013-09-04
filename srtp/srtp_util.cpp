#include "srtp_util.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <openssl/sha.h>
#include <sys/socket.h>

#include <sys/time.h> // for struct timeval & gettimeofday

#include <fcntl.h> // fcntl, F_SETFL

#include "srtp_debug.h"

ssize_t srtp_read(int sock, char* buffer, size_t length);
ssize_t srtp_write(int sock, char* buffer, size_t length);
ssize_t srtp_sendto(int sock, char* message, size_t length, int flags, struct sockaddr* dest_addr, socklen_t dest_len);

int handle_data_pkt( srtp_header_t* head, Conn_t* conn );
int handle_cmd_pkt( srtp_header_t* head, Conn_t* conn );

int fdByAddrHash( unsigned char* data, size_t len );

std::map<std::string, int> hash2fd;

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

    std::list<Mesg_t>::iterator pos = conn->inbox.end();

    for (std::list<Mesg_t>::iterator it = conn->inbox.begin();

      it != conn->inbox.end(); it++ ){

      if ( it->seq > head->seq ){
        pos = it;
        break;
      }

    }

    // Insert mesg before pos unless pos is end() then insert will act as push back
    conn->inbox.insert(pos, { mesg_buf, head->len, head->seq } );

  }

  return 0;
}

int handle_cmd_pkt( srtp_header_t* head, Conn_t* conn ){

  // Connection Request
  if ( ( head->cmd & SYN ) && ( head->seq == 0 ) ) {
    //sendto ack
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

    int fifo = fdByAddrHash( ( unsigned char* ) addr, ( size_t ) addr_len );

    Conn_t* conn = fd2conn[ fifo ];

    if ( header->len > 0 ) { // Data Packet

      ( void ) handle_data_pkt( header, conn );

      // Send syn ack

      srtp_header_t ack_pkt;

      ack_pkt.seq = header->seq;
      ack_pkt.cmd = ACK;
      ack_pkt.len = 0;

      // send_ack_to(sock, seq, addr, addr_len);

      ( void ) srtp_sendto( sock, ( char* ) &ack_pkt, PKT_HEADERSIZE, 0, addr, *addr_len );

      continue;

    }

    // Command Packet

    ( void ) handle_cmd_pkt( header, conn );

    // send_ack_to(...)

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

int fdByAddrHash( unsigned char* data, size_t len ){

  unsigned char raw[ 20 ];
  char str[ 40 ];

  ( void ) SHA1( ( unsigned char* )data, len, raw );

  for(int i = 0; i < 20; i++){
    sprintf(&str[2*i], "%x", raw[i]);
  }

  std::string shasum( ( char* ) str, ( size_t ) 40 );

  if ( hash2fd.find( shasum ) == hash2fd.end() ){
    return -1;
  }

  return hash2fd[ shasum ];

}
