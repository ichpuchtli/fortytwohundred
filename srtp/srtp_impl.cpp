#include "srtp_impl.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

// C++ Headers
#include <map>

std::map<int, char*> fd2fifo;

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
  
  fd2fifo.insert({fd, filename});
  
  return fd;
  
}

int _srtp_listen( int socket, int backlog ){return 0;}

int _srtp_bind( int socket, const struct sockaddr* address, socklen_t address_len ){return 0;}

int _srtp_accept( int socket, struct sockaddr* address, socklen_t* address_len ){return 0;}

int _srtp_connect( int socket, const struct sockaddr* address, socklen_t address_len ){return 0;}

int _srtp_shutdown( int socket, int how ){return 0;}

int _srtp_getpeername( int socket, struct sockaddr* address, socklen_t* address_len ){return 0;}

int _srtp_send( int socket, const void* message, size_t length, int flags ){return 0;}

int _srtp_recv( int socket, void* buffer, size_t length, int flags ){return 0;}

