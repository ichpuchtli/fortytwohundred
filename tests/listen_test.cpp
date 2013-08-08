#include "gtest/gtest.h"

#include "../srtp/srtp.h"

#define TEST_MESSAGE "Hello World!\r\n"
#define TEST_PORT ( 4000 )

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

void* echo_client( void* param ){

  int n, sock_fd;
  char buffer[ 64 ];
  ( void ) param;

  sock_fd = IPv4_connect( TEST_PORT, "127.0.0.1" ); 

  if ( sock_fd < 0 ){
    perror( "echo client" );
    return NULL;
  }

  n = read ( sock_fd, buffer, 64 );

  write ( sock_fd, buffer, n );

  close( sock_fd );

  return NULL;
}


TEST( ListenTest, IPv6BoundListen ) {

  int sock_fd;

  sock_fd = srtp_socket(AF_INET6, SOCK_STREAM, 0);

  reuse_addr( sock_fd );

  ASSERT_TRUE( sock_fd > 0 );

  struct sockaddr_in6 addr;
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons ( TEST_PORT );

  inet_pton( AF_INET6, "::1", &addr.sin6_addr );

  int bind_result = srtp_bind( sock_fd, ( struct sockaddr* ) &addr, sizeof( struct sockaddr_in6 ) );

  EXPECT_TRUE( bind_result == 0 );

  int listen_result = srtp_listen( sock_fd, 0 );

  EXPECT_TRUE( listen_result == 0 );

  close( sock_fd );

}

TEST( ListenTest, IPv4BoundListen ) {

  int sock_fd;

  sock_fd = srtp_socket(AF_INET, SOCK_STREAM, 0);

  reuse_addr( sock_fd );

  ASSERT_TRUE( sock_fd > 0 );

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons ( TEST_PORT );
  addr.sin_addr.s_addr = htonl ( INADDR_ANY );

  int bind_result = srtp_bind( sock_fd, ( struct sockaddr* ) &addr, sizeof( struct sockaddr_in ) );

  EXPECT_TRUE( bind_result == 0 );

  int listen_result = srtp_listen( sock_fd, 0 );

  EXPECT_TRUE( listen_result == 0 );

  close( sock_fd );

}

TEST(ListenTest, Argument_Test) {

  EXPECT_TRUE( srtp_listen( -1, 0 ) < 0 );

  EXPECT_TRUE( srtp_listen( -1, -1 ) < 0 );
}

TEST( ListenTest, ClientServerCommunication ) {

  pthread_t id;

  int sock_fd;

  sock_fd = srtp_socket(AF_INET, SOCK_STREAM, 0);

  reuse_addr( sock_fd );

  ASSERT_TRUE( sock_fd > 0 );

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons ( TEST_PORT );
  addr.sin_addr.s_addr = htonl ( INADDR_ANY );

  int bind_result = srtp_bind( sock_fd, ( struct sockaddr* ) &addr, sizeof( struct sockaddr_in ) );

  ASSERT_TRUE( bind_result == 0 );

  int listen_result = srtp_listen( sock_fd, 0 );

  ASSERT_TRUE( listen_result == 0 );

  pthread_create( &id, NULL, echo_client, NULL );

  struct sockaddr_in client_addr;
  socklen_t socklen = sizeof( struct sockaddr_in );

  int client_fd = srtp_accept( sock_fd, ( struct sockaddr* ) &client_addr, &socklen );

  ASSERT_TRUE(client_fd > 0 );

  char buffer[ 64 ];

  write( client_fd, TEST_MESSAGE, strlen( TEST_MESSAGE ) );

  read( client_fd, buffer, 64 );

  EXPECT_STREQ( TEST_MESSAGE, buffer );

  pthread_join( id, NULL );

  if ( sock_fd > 0 ) close( sock_fd );
}
