#include "srtp_util.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <sys/time.h> // for struct timeval & gettimeofday

#include <fcntl.h> // fcntl, F_SETFL

#include "srtp_debug.h"

ssize_t srtp_read(int sock, char* buffer, size_t length);
ssize_t srtp_write(int sock, char* buffer, size_t length);
ssize_t srtp_sendto(int sock, char* message, size_t length, int flags, struct sockaddr* dest_addr, socklen_t dest_len);
ssize_t srtp_recvfrom(int sock, char* buffer, size_t length, int flags, struct sockaddr* addr, socklen_t* addr_len);


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

int recv_srtp_data(int sock, char* buffer, size_t len, struct sockaddr* addr, socklen_t* addr_len){

  int n;

  n = recvfrom( sock, buffer, len, 0, addr, addr_len );

  return n;
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
    PKT_SETSEQ(buff, ); //TODO how will we count sequence nums??
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
  sprintf(s, "|%s|%s|%s|%s|",
    (cmd&RST?"RST":"."),
    (cmd&FIN?"FIN":"."),
    (cmd&ACK?"ACK":"."),
    (cmd&SYN?"SYN":"."));
}

void srtp_pktstr(char* buf, char* s) {
    
}

int pkt_invalid(char* buf) {
  if (PKT_INVALID_LEN(buf)) {
    debug("[pkt_invalid] PKT_INVALID_LEN(%u)\n", len);
    return 1;
  }
  if (PKT_INVALID_CMD(buf)) {
    debug("[pkt_invalid] PKT_INVALID_CMD(%u)\n", len);
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

int pkt_pack(char* buf, uint8_t cmd, uint16_t len, uint16_t seq, uint16_t ack, uint8_t checksum, char* payload) {
  if (PKT_INVALID_CMD(cmd)) {
    debug("[pkt_set_payload] PKT_INVALID_LEN(%u)\n", len);
    return -1;
  }
  PKT_SETCMD(buf, cmd);
  PKT_SETSEQ(buf, seq);
  PKT_SETACK(buf, ack);
  PKT_SETCHECKSUM(buf, checksum);
  
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

ssize_t srtp_recvfrom(int sock, char* buffer, size_t length, int flags, struct sockaddr* addr, socklen_t* addr_len) {
  
  ssize_t recvd = 0;
  
  do{
    recvd += recvfrom(sock, &buffer[recvd], length - recvd, flags, addr, addr_len);
  } while(recvd > 0 && (size_t) recvd < length);
    
  return recvd;
    
}