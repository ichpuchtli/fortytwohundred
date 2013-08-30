#include "gtest/gtest.h"

#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"

#include "../srtp/srtp.h"

#define TEST_MESSAGE "Hello World!\r\n"
#define TEST_MESG_LEN (14)
#define TEST_PORT ( 8080 )

char* loopback_addr( void ) {
  char* addr = getenv( "IP" );

  return ( addr ) ? addr : ( char* ) "127.0.0.1" ;
}

void reuse_addr( int sock ){
  int tmp = 1;
  setsockopt ( sock, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof ( int ) );
}

int IPv4_connect( int port, char* host ){

  int sock_fd = srtp_socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons ( port );
  addr.sin_addr.s_addr = inet_addr( host );

  int connect_result = srtp_connect( sock_fd, ( struct sockaddr * ) &addr, sizeof( struct sockaddr ) );

  return ( connect_result == 0 ) ? sock_fd : -1; 
}

void* write_client( void* param ){

  int n, sock_fd;
  char buffer[ 64 ];
  ( void ) param;

  sock_fd = IPv4_connect( TEST_PORT, loopback_addr() );

  if ( sock_fd < 0 ){
    perror( "echo client" );
    return NULL;
  }

  write ( sock_fd, TEST_MESSAGE, strlen(TEST_MESSAGE) );

  srtp_shutdown( sock_fd , 0 );

  return NULL;
}

TEST( Client2Server, OneClientOneServer ) {

  pthread_t id;

  int sock_fd;

  sock_fd = srtp_socket(AF_INET, SOCK_STREAM, 0);

  //reuse_addr( sock_fd );

  ASSERT_GT( sock_fd, 0 );

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons ( TEST_PORT );
  addr.sin_addr.s_addr = inet_addr( loopback_addr() );

  int bind_result = srtp_bind( sock_fd, ( struct sockaddr* ) &addr, sizeof( struct sockaddr_in ) );

  ASSERT_EQ( bind_result, 0 );

  int listen_result = srtp_listen( sock_fd, 0 );

  ASSERT_EQ( listen_result, 0 );

  pthread_create( &id, NULL, write_client, NULL );

  struct sockaddr_in client_addr;
  socklen_t socklen = sizeof( struct sockaddr_in );

  int client_fd = srtp_accept( sock_fd, ( struct sockaddr* ) &client_addr, &socklen );

  ASSERT_GT(client_fd, 0 );

  char buffer[ 64 ];
  memset(buffer, '\0', 64);

  read( client_fd, buffer, 64 );

  EXPECT_STREQ( TEST_MESSAGE, buffer );

  pthread_join( id, NULL ); // client_write

  shutdown( sock_fd , 0); // server

  close( client_fd ); // fifo to read from server

}

TEST( Client2Server, ManyClientOneServer ) {

  pthread_t id[10];

  int sock_fd;

  sock_fd = srtp_socket(AF_INET, SOCK_STREAM, 0);

  //reuse_addr( sock_fd );

  ASSERT_GT( sock_fd, 0 );

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons ( TEST_PORT );
  addr.sin_addr.s_addr = inet_addr( loopback_addr() );

  int bind_result = srtp_bind( sock_fd, ( struct sockaddr* ) &addr, sizeof( struct sockaddr_in ) );

  ASSERT_EQ( bind_result, 0 );

  int listen_result = srtp_listen( sock_fd, 0 );

  ASSERT_EQ( listen_result, 0 );

  for(int i = 0; i < 2; i++){
    pthread_create( &id[i], NULL, write_client, NULL );
  }

  struct sockaddr_in client_addr;
  socklen_t socklen = sizeof( struct sockaddr_in );

  int client_fd = srtp_accept( sock_fd, ( struct sockaddr* ) &client_addr, &socklen );

  ASSERT_GT(client_fd, 0 );

  char buffer[ 64 ];
  memset(buffer, '\0', 64);

  int n;

  n = read( client_fd, buffer, 64 );
  write( STDERR_FILENO, buffer, n );
  EXPECT_STREQ( TEST_MESSAGE, buffer );

  close( client_fd );

  client_fd = srtp_accept( sock_fd, ( struct sockaddr* ) &client_addr, &socklen );

  ASSERT_GT(client_fd, 0 );

  n = read( client_fd, buffer, 64 );
  write( STDERR_FILENO, buffer, n );
  EXPECT_STREQ( TEST_MESSAGE, buffer );

  close( client_fd );

  for(int i = 0; i < 2; i++){
    pthread_join( id[i], NULL );
  }

  shutdown( sock_fd , 0);

}
