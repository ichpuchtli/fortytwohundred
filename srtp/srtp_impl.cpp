#include "srtp_impl.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

// C++ Headers
#include <map>
#include <string>

// fifo_fd -> fifo_path
std::map<int, std::string> fd2fifo;

// sha1 hash(addr) -> fifo_fd 
std::map<std::string, int> id2fd;

const int FIFO_PERMISSIONS = 0666;

// TODO map fds to fifofilename
int _srtp_socket( int domain, int type, int protocol ){

  static int fifo_id = 0;
  
  char filename[32];
  
  sprintf(filename, "tmpfifo%05d", fifo_id++);
  
  int errcode = mkfifo(filename, FIFO_PERMISSIONS );
  
  if(errcode){
    perror("_srtp_socket[mkfifo]");
    return -1;
  }
  
  int fd = open(filename, O_RDWR);
  
  if(fd < 0){
    perror("_srtp_socket[open]");
    unlink(filename);
    return -1;
  }
  
  fd2fifo.insert({fd, std::string(filename)});
  
  return fd;
  
}

int _srtp_listen( int socket, int backlog ){
  
  // detach event loop thread
  return 0;
}

int _srtp_bind( int socket, const struct sockaddr* address, socklen_t address_len ){
  
  // id = hash(address)
  // id2fd.insert(id, socket);
  return 0;
  
}

int _srtp_accept( int socket, struct sockaddr* address, socklen_t* address_len ){
  
  // wait/block for unique client connection
  // create fifo
  // append fifo to list of active connections
  // return fifo_fd
  return 0;

}

int _srtp_connect( int socket, const struct sockaddr* address, socklen_t address_len ){
  
  // request connection
  // wait for ack
  // detach event loop thread
  // create fifo
  // append fifo to list of active connections
  // return fifo_fd
  
  return 0;

}

int _srtp_shutdown( int socket, int how ){
  
  // send shutdown packet
  // stop event loop
  // join event loop thread
  // unlink fifo
  
  return 0;
}

int _srtp_getpeername( int socket, struct sockaddr* address, socklen_t* address_len ){return 0;}

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