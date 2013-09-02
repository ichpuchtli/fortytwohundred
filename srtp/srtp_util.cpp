#include "srtp_util.h"
#include "srtp_debug.h"

int establish_conn(int sock, const struct sockaddr* addr, socklen_t addr_len){

  (void) sock;
  (void) addr;
  (void) addr_len;

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

int send_srtp_data(int sock, char* data, size_t len, const struct sockaddr* addr, socklen_t addr_len ){

  (void) sendto(sock, data, len, 0, addr, addr_len);

  return 0;
}
